/*
 * (C) 2015 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <margo.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>

#include "sdskv-client.h"

static std::string gen_random_string(size_t len);

int main(int argc, char *argv[])
{
    char cli_addr_prefix[64] = {0};
    char *sdskv_svr_addr_str;
    char *db_name;
    margo_instance_id mid;
    hg_addr_t svr_addr;
    uint8_t mplex_id;
    uint32_t num_keys;
    sdskv_client_t kvcl;
    sdskv_provider_handle_t kvph;
    hg_return_t hret;
    int ret;

    if(argc != 5)
    {
        fprintf(stderr, "Usage: %s <sdskv_server_addr> <mplex_id> <db_name> <num_keys>\n", argv[0]);
        fprintf(stderr, "  Example: %s tcp://localhost:1234 1 foo 1000\n", argv[0]);
        return(-1);
    }
    sdskv_svr_addr_str = argv[1];
    mplex_id           = atoi(argv[2]);
    db_name            = argv[3];
    num_keys           = atoi(argv[4]);

    /* initialize Margo using the transport portion of the server
     * address (i.e., the part before the first : character if present)
     */
    for(unsigned i=0; (i<63 && sdskv_svr_addr_str[i] != '\0' && sdskv_svr_addr_str[i] != ':'); i++)
        cli_addr_prefix[i] = sdskv_svr_addr_str[i];

    /* start margo */
    mid = margo_init(cli_addr_prefix, MARGO_SERVER_MODE, 0, 0);
    if(mid == MARGO_INSTANCE_NULL)
    {
        fprintf(stderr, "Error: margo_init()\n");
        return(-1);
    }

    ret = sdskv_client_init(mid, &kvcl);
    if(ret != 0)
    {
        fprintf(stderr, "Error: sdskv_client_init()\n");
        margo_finalize(mid);
        return -1;
    }

    /* look up the SDSKV server address */
    hret = margo_addr_lookup(mid, sdskv_svr_addr_str, &svr_addr);
    if(hret != HG_SUCCESS)
    {
        fprintf(stderr, "Error: margo_addr_lookup()\n");
        sdskv_client_finalize(kvcl);
        margo_finalize(mid);
        return(-1);
    }

    /* create a SDSKV provider handle */
    ret = sdskv_provider_handle_create(kvcl, svr_addr, mplex_id, &kvph);
    if(ret != 0)
    {
        fprintf(stderr, "Error: sdskv_provider_handle_create()\n");
        margo_addr_free(mid, svr_addr);
        sdskv_client_finalize(kvcl);
        margo_finalize(mid);
        return(-1);
    }

    /* open the database */
    sdskv_database_id_t db_id;
    ret = sdskv_open(kvph, db_name, &db_id);
    if(ret == 0) {
        printf("Successfuly open database %s, id is %ld\n", db_name, db_id);
    } else {
        fprintf(stderr, "Error: could not open database %s\n", db_name);
        sdskv_provider_handle_release(kvph);
        margo_addr_free(mid, svr_addr);
        sdskv_client_finalize(kvcl);
        margo_finalize(mid);
        return(-1);
    }

    /* **** put keys ***** */
    std::vector<std::string> keys;
    std::map<std::string, std::string> reference;
    size_t max_value_size = 8000;
    size_t max_key_size = 16;

    for(unsigned i=0; i < num_keys; i++) {
        auto k = gen_random_string((max_key_size+(rand()%max_key_size))/2);
        // half of the entries will be put using bulk
        auto v = gen_random_string(i*max_value_size/num_keys);
        ret = sdskv_put(kvph, db_id,
                (const void *)k.data(), k.size(),
                (const void *)v.data(), v.size());
        if(ret != 0) {
            fprintf(stderr, "Error: sdskv_put() failed (iteration %d)\n", i);
            sdskv_shutdown_service(kvcl, svr_addr);
            sdskv_provider_handle_release(kvph);
            margo_addr_free(mid, svr_addr);
            sdskv_client_finalize(kvcl);
            margo_finalize(mid);
            return -1;
        }
        keys.push_back(k);
        reference[k] = v;
        std::cout << k << " ===> " << v << std::endl;
    }
    printf("Successfuly inserted %d keys\n", num_keys);

    /* **** list keys **** */
    std::sort(keys.begin(), keys.end());
    auto i1 = keys.size()/3;
    auto i2 = 2*keys.size()/3;
    auto keys_after = keys[i1-1];
    hg_size_t max_keys = i2-i1;

    std::vector<std::vector<char>> key_strings(max_keys, std::vector<char>(max_key_size));
    std::vector<std::vector<char>> val_strings(max_keys, std::vector<char>(max_value_size));
    std::vector<void*> keys_addr(max_keys);
    std::vector<void*> vals_addr(max_keys);
    std::vector<hg_size_t> ksizes(max_keys, max_key_size+1);
    std::vector<hg_size_t> vsizes(max_keys, max_value_size+1);

    for(unsigned i=0; i<max_keys; i++) {
        keys_addr[i] = (void*)key_strings[i].data();
        vals_addr[i] = (void*)val_strings[i].data();
    }

    ret = sdskv_list_keyvals(kvph, db_id,
                (const void*)keys_after.c_str(), keys_after.size(),
                keys_addr.data(), ksizes.data(), 
                vals_addr.data(), vsizes.data(),
                &max_keys);

    if(ret != 0) {
        fprintf(stderr, "Error: sdskv_list_keyvals() failed\n");
        sdskv_shutdown_service(kvcl, svr_addr);
        sdskv_provider_handle_release(kvph);
        margo_addr_free(mid, svr_addr);
        sdskv_client_finalize(kvcl);
        margo_finalize(mid);
        return -1;
    }

    if(max_keys != i2-i1) {
        fprintf(stderr, "Error: number of returned keys (%ld) is not the number requested (%ld)\n", max_keys, i2-i1);
        sdskv_shutdown_service(kvcl, svr_addr);
        sdskv_provider_handle_release(kvph);
        margo_addr_free(mid, svr_addr);
        sdskv_client_finalize(kvcl);
        margo_finalize(mid);
        return -1;
    }

    /* put the returned values in an array */
    std::vector<std::string> res_k, res_v;
    for(auto ptr : vals_addr) {
        res_v.push_back(std::string((const char*)ptr));
    }
    for(auto ptr : keys_addr) {
        res_k.push_back(std::string((const char*)ptr));
    }

    /* check that the returned keys are correct */
    for(unsigned i=0; i < max_keys; i++) {
        if(res_k[i] != keys[i+i1]) {
            fprintf(stderr, "Error: returned keys don't match expected keys\n");
            fprintf(stderr, "       key received: %s\n", res_k[i].c_str());
            fprintf(stderr, "       key expected: %s\n", keys[i+i1].c_str());
            sdskv_shutdown_service(kvcl, svr_addr);
            sdskv_provider_handle_release(kvph);
            margo_addr_free(mid, svr_addr);
            sdskv_client_finalize(kvcl);
            margo_finalize(mid);
            return -1;
        }
    }

    /* check that the returned values are correct */
    for(unsigned i=0; i < max_keys; i++) {
        if(reference[res_k[i]] == res_v[i]) {
            fprintf(stderr, "value %d received matches expected value: %s\n", i, res_v[i].c_str());
        } else {
            fprintf(stderr, "Error: returned value %d don't match expected value\n", i);
            fprintf(stderr, "   expected: %s\n   received: %s\n", reference[res_k[i]].c_str(), res_v[i].c_str());
            sdskv_shutdown_service(kvcl, svr_addr);
            sdskv_provider_handle_release(kvph);
            margo_addr_free(mid, svr_addr);
            sdskv_client_finalize(kvcl);
            margo_finalize(mid);
            return -1;
        }
    }

    /* shutdown the server */
    ret = sdskv_shutdown_service(kvcl, svr_addr);

    /**** cleanup ****/
    sdskv_provider_handle_release(kvph);
    margo_addr_free(mid, svr_addr);
    sdskv_client_finalize(kvcl);
    margo_finalize(mid);
    return(ret);
}

static std::string gen_random_string(size_t len) {
    static const char alphanum[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
    std::string s(len, ' ');
    for (unsigned i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    return s;
}
