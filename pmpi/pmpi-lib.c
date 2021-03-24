#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ssg.h>
#include <ssg-mpi.h>


#include "pmpi-lib.h"
#include "my-util.h"

#include <margo.h>
#include "../include/sdskv-client.h"
#include "../include/sdskv-server.h"
#include "../include/sdskv-common.h"


const unsigned num_keys = 5;
static const char* keys[] = {"MPI_Init",
                             "MPI_Send",
                             "MPI_Isend",
                             "MPI_Recv",
                             "MPI_Finalize"};

static unsigned int num_init = 0;
static unsigned int num_send = 0;
static unsigned int num_isend = 0;
static unsigned int num_recv = 0;
static unsigned int num_finalize = 0;


static sdskv_provider_handle_t kvph;
static sdskv_database_id_t db_id;
static sdskv_client_t kvcl;
static hg_addr_t svr_addr;
static margo_instance_id mid;




static void my_membership_update_cb(void* uargs,
                                    ssg_member_id_t member_id,
                                    ssg_member_update_type_t update_type)
{
    switch(update_type) {
    case SSG_MEMBER_JOINED:
        printf("Member %ld joined\n", member_id);
        break;
    case SSG_MEMBER_LEFT:
        printf("Member %ld left\n", member_id);
        break;
    case SSG_MEMBER_DIED:
        printf("Member %ld died\n", member_id);
        break;
    }

    return 0;
}
/**
 * Puts a key-value pair in the sdskv database and checks
 * for errors. If any errors happen during the sdskv_put,
 * this function shuts down and frees the server, and returns
 * -1, otherwise returns the return value from sdskv_put.
 */
int sdskv_put_check_err(sdskv_provider_handle_t provider,
                        sdskv_database_id_t db_id,
                        const void *key, hg_size_t ksize,
                        const void *value, hg_size_t vsize)
{
    int ret;
    //printf("sdskv_put_check_err: key = %s\n", (char*) key);
    //printf("sdskv_put_check_err: ksize = %d\n", ksize);
    //printf("sdskv_put_check_err: value = %d\n", *((int*) value));
    //printf("sdskv_put_check_err: vsize = %d\n", vsize);
    //printf("kvph = %d\n", kvph);
    //printf("db_id = %d\n", db_id);
    int rank;
    PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("Rank %d, putting kv: %s => %d\n", rank, (char*) key, *(int*) value);
    ret = sdskv_put(kvph, db_id,
                    (const void*) key, ksize,
                    (const void*) value, vsize);
    if(ret != 0) {
        printf("Error: Rank %d sdskv_put() failed for kv pair (%s, %d). ret = %X\n",
                rank, (char*) key, *(int*) value, ret);
        printf("Rank %d will not do anything about it...\n", rank);
        //sdskv_shutdown_service(kvcl, svr_addr);
        //sdskv_provider_handle_release(kvph);
        //margo_addr_free(mid, svr_addr);
        //sdskv_client_finalize(kvcl);
        //margo_finalize(mid);
        //return -1;
    } else {
        printf("Rank %d: ret is %X\n", rank, ret);
    }

    return ret;
}


int MPI_Init(int *argc, char ***argv)
{
    int mpi_init_ret;
    int ssg_ret;

    //print_args(argc, argv, "MPI_Init");

    if(*argc != 5)
    {
        fprintf(stderr, "MPI_Init: Usage: %s <sdskv_server_addr> <mplex_id> <db_name> <num_keys>\n",
                (*argv)[0]);
        fprintf(stderr, "  Example: %s tcp://localhost:1234 1 foo 1000\n", (*argv)[0]);
        return(-1);
    }

    mpi_init_ret = PMPI_Init(argc, argv);
    // these need to be the args to init_marg_open_db_check_error()
    //sdskv_svr_addr_str = (*argv)[1];
    //printf("the server address is %s\n", sdskv_svr_addr_str);
    //mplex_id          = atoi((*argv)[2]);
    //db_name           = (*argv)[3];
    //num_keys          = atoi((*argv)[4]);
    init_margo_open_db_check_error(argc, argv);


    //PMPI_Barrier(MPI_COMM_WORLD);
    return mpi_init_ret;
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
             int tag, MPI_Comm comm)
{
    int ret;

    ret = PMPI_Send(buf, count, datatype, dest, tag, comm);
    num_send++;
    // TODO send to db and vectors
    return ret;
}

int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm, MPI_Request *request)
{
    int ret;

    ret = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
    num_isend++;
    // TODO send to db and vectors
    return ret;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype,
             int source, int tag, MPI_Comm comm, MPI_Status *status)
{
    int ret;

    ret = PMPI_Recv(buf, count, datatype, source, tag, comm, status);
    num_recv++;


    // TODO recv to db and vectors
    return ret;
}

int MPI_Finalize()
{
    int pmpi_finalize_ret = 0;

    int rank;
    PMPI_Barrier(MPI_COMM_WORLD);
    PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
    //printf("In MPI_Finalize, rank = %d: Number of MPI_Init() called : %d\n",rank, num_init);
    //printf("In MPI_Finalize, rank = %d: Number of MPI_Send() called : %d\n",rank, num_send);
    //printf("In MPI_Finalize, rank = %d: Number of MPI_Isend() called : %d\n", rank, num_isend);
    //printf("In MPI_Finalize, rank = %d: Number of MPI_Recv() called : %d\n",rank, num_recv);
    //printf("In MPI_Finalize, rank = %d: Number of MPI_Finalize() called : %d\n",rank, num_finalize);


    // This block should eventually be removed and each
    // MPI routine should have its own sdskv_put_check_err
    unsigned int data[num_keys] = {num_init, num_send, num_isend, num_recv, num_finalize};
    unsigned i;
    //printf("Test: sizeof(char)*strlen(keys[i]) = %lu\n", sizeof(char)*strlen(keys[0]));
    //printf("Test: keys[0] = '%s', &keys[0]   = %#x\n", keys[0], &keys[0]);
    //printf("Test:                (&keys)[0]  = %#x\n", (&keys)[0]);
    //printf("Test:                (&keys[0])  = %#x\n", &(keys[0]));
    char* key = NULL;
    unsigned ksize = 0;
    unsigned datum = 0;
    unsigned dsize = 0;
    for (i = 0; i < num_keys; i++) {
        key = keys[i];
        ksize = sizeof(char)*strlen(keys[i]);
        //datum = data[i];
        dsize = sizeof(data[i]);
	unsigned int global_data;
	PMPI_Reduce(&(data[i]), &global_data, 1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_WORLD);
	if (rank == 0) {
	    sdskv_put_check_err(kvph, db_id,
				(const void*) key, ksize,
				//(const void*) &datum, dsize);
				(const void*) &(global_data), dsize);
	}
	PMPI_Barrier(MPI_COMM_WORLD);
    }

    pmpi_finalize_ret = PMPI_Finalize();
    return pmpi_finalize_ret;
}



int sdskv_shutdown_service_cleanup() {
    int ret = 0;
    int rank;
    PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("Rank %d is shutting down sdskv service...\n", rank);
    /* shutdown the server */
    ret = sdskv_shutdown_service(kvcl, svr_addr);

    /**** cleanup ****/
    sdskv_provider_handle_release(kvph);
    margo_addr_free(mid, svr_addr);
    sdskv_client_finalize(kvcl);
    margo_finalize(mid);

    return ret;
}


int init_margo_open_db_check_error(int* argc, char*** argv) {
    int ret;
    int ssg_ret;

    char cli_addr_prefix[64] = {0};
    char *sdskv_svr_addr_str;
    char *db_name;
    hg_addr_t svr_addr;
    uint8_t mplex_id;
    uint32_t num_keys;
    hg_return_t hret;

    sdskv_svr_addr_str = (*argv)[1];
    printf("the server address is %s\n", sdskv_svr_addr_str);
    mplex_id          = atoi((*argv)[2]);
    db_name           = (*argv)[3];
    num_keys          = atoi((*argv)[4]);


    int rank;
    PMPI_Comm_rank(MPI_COMM_WORLD, &rank);


    /* initialize Margo using the transport portion of the server
     * address (i.e., the part before the first : character if present)
     */
    for(unsigned i=0; (i<63 && sdskv_svr_addr_str[i] != '\0' && sdskv_svr_addr_str[i] != ':'); i++)
        cli_addr_prefix[i] = sdskv_svr_addr_str[i];

    /* Create the MPI group */
    //ssg_ret = ssg_init();
    //assert(ssg_ret == SSG_SUCCESS);

    /* start margo */
    // May have to use '1' in the third argument to create
    // a new ES for the ULts. The fourth argument needs MAY need to
    // be either -1 or a postive value.
    mid = margo_init(cli_addr_prefix, MARGO_SERVER_MODE, 0, 0);
    if(mid == MARGO_INSTANCE_NULL)
    {
        printf("Rank %d, Error: margo_init()\n", rank);
        return(-1);
    } else {
        printf("Rank %d, margo_init() success\n", rank);
    }
    PMPI_Barrier(MPI_COMM_WORLD);





    //ssg_group_config_t config = {
    //    .swim_period_length_ms = 1000,
    //    .swim_suspect_timeout_periods = 5,
    //    .swim_subgroup_member_count = -1,
    //    .ssg_credential = -1
    //};

    //ssg_group_id_t gid = ssg_group_create_mpi(
    //    mid, "mygroup", MPI_COMM_WORLD,
    //    &config, my_membership_update_cb, NULL);





    ret = sdskv_client_init(mid, &kvcl);
    if(ret != 0)
    {
        fprintf(stderr, "Rank %d, Error: sdskv_client_init()\n", rank);
        margo_finalize(mid);
        return -1;
    } else {
        printf("Rank %d: sdskv_client_init() success\n", rank);
    }
    PMPI_Barrier(MPI_COMM_WORLD);

    /* look up the SDSKV server address */
    hret = margo_addr_lookup(mid, sdskv_svr_addr_str, &svr_addr);
    if(hret != HG_SUCCESS)
    {
        printf("Rank %d, Error: margo_addr_lookup()\n", rank);
        sdskv_client_finalize(kvcl);
        margo_finalize(mid);
        return(-1);
    } else {
        printf("Rank %d: margo_addr_lookup() success\n", rank);
    }
    PMPI_Barrier(MPI_COMM_WORLD);

    /* create a SDSKV provider handle */
    ret = sdskv_provider_handle_create(kvcl, svr_addr, mplex_id, &kvph);
    if(ret != 0)
    {
        printf("Rank %d, Error: sdskv_provider_handle_create()\n", rank);
        margo_addr_free(mid, svr_addr);
        sdskv_client_finalize(kvcl);
        margo_finalize(mid);
        return(-1);
    } else {
        printf("Rank %d: sdskv_provider_handle_create() success\n", rank);
        printf("Rank %d: kvph = %d\n", rank, kvph);
    }
    PMPI_Barrier(MPI_COMM_WORLD);

    /* open the database */
    ret = sdskv_open(kvph, db_name, &db_id);
    if(ret == 0) {
        printf("Rank %d: Successfully opened database %s, id is %ld\n", rank, db_name, db_id);
        printf("Rank %d: db_id = %d\n", rank, db_id);
    } else {
        printf("Rank %d: Error: could not open database %s\n", rank, db_name);
        printf("Rank %d: db_id = %d\n", rank, db_id);
        printf("Rank %d: sdskv_open return value was %d\n", rank, ret);
        printf("Rank %d: error message was: %s\n", rank, sdskv_error_messages[ret]);
        printf("Rank %d will not doing anything about error...\n", rank);
        //sdskv_provider_handle_release(kvph);
        //margo_addr_free(mid, svr_addr);
        //sdskv_client_finalize(kvcl);
        //margo_finalize(mid);
        //return(-1);
    }
    PMPI_Barrier(MPI_COMM_WORLD);

    return ret;
}
