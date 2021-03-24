#ifndef PMPI_LIB_H
#define PMPI_LIB_H

#include <margo.h>
#include "../include/sdskv-client.h"
#include "../include/sdskv-server.h"
#include "../include/sdskv-common.h"

/**
 * Puts a key-value pair in the sdskv database and checks
 * for errors. If any errors happen during the sdskv_put,
 * this function shuts down and frees the server, and returns
 * -1, otherwise returns the return value from sdskv_put.
 */
int sdskv_put_check_err(sdskv_provider_handle_t provider,
                        sdskv_database_id_t db_id,
                        const void *key, hg_size_t ksize,
                        const void *value, hg_size_t vsize);


int sdskv_shutdown_service_cleanup();

int init_margo_open_db_check_error();

static void my_membership_update_cb(void* uargs,
				    ssg_member_id_t member_id,
				    ssg_member_update_type_t update_type);


/**
 * Initialize Margo and SDSKV first, then call MPI_Init
 **/
int MPI_Init(int *argc, char ***argv);

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
             int tag, MPI_Comm comm);

int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm, MPI_Request *request);

int MPI_Recv(void *buf, int count, MPI_Datatype datatype,
             int source, int tag, MPI_Comm comm, MPI_Status *status);

int MPI_Finalize();


#endif
