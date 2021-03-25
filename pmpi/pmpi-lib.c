#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ssg.h>
#include <ssg-mpi.h>


#include "pmpi-lib.h"
#include "pmpi-common.h"

#include <margo.h>
#include "../include/sdskv-client.h"
#include "../include/sdskv-server.h"
#include "../include/sdskv-common.h"


static unsigned long num_init = 0;
static unsigned long num_send = 0;
static unsigned long num_isend = 0;
static unsigned long num_recv = 0;
static unsigned long num_finalize = 0;

static unsigned int threshold_init = 5;
static unsigned int threshold_send = 5;
static unsigned int threshold_isend = 5;
static unsigned int threshold_recv = 5;
static unsigned int threshold_finalize = 5;

int MPI_Init(int *argc, char ***argv)
{
    int mpi_init_ret;
    int ssg_ret;


    if(*argc != 5)
    {
        fprintf(stderr, "MPI_Init: Usage: %s <sdskv_server_addr> <mplex_id> <db_name> <num_keys>\n",
                (*argv)[0]);
        fprintf(stderr, "  Example: %s tcp://localhost:1234 1 foo 1000\n", (*argv)[0]);
        return(-1);
    }

    mpi_init_ret = PMPI_Init(argc, argv);
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


    // This block should eventually be removed and each
    // MPI routine should have its own sdskv_put_check_err
    //unsigned int data[num_keys] = {num_init, num_send, num_isend, num_recv, num_finalize};
    unsigned long data[total_keys] = {num_init, num_send, num_isend, num_recv, num_finalize};
 										   
    unsigned i;
    unsigned ksize = 0;
    unsigned dsize = 0;
    for (i = 0; i < total_keys; i++) {
        ksize = sizeof(char)*strlen(keys[i]);
        dsize = sizeof(unsigned long);
	unsigned long global_data = 0;
	PMPI_Reduce(&(data[i]), &global_data, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
	if (rank == 0) {
	    printf("Rank %d: global_data is %lu for key %s\n", 0, global_data, keys[i]);
	    sdskv_put_check_err( (const void*) keys[i], ksize,
	    			 (const void*) &global_data, dsize);
	}
	PMPI_Barrier(MPI_COMM_WORLD);
    }

    pmpi_finalize_ret = PMPI_Finalize();
    return pmpi_finalize_ret;
}


