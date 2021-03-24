CC = mpicc

FLAGS_MAIN = -Wall -g -o main-generate
LIBS_MAIN = -lmargo -lmercury -labt

FLAGS_PMPI = -Wall -g -shared -o pmpi-lib.so 
LIBS_PMPI = -lmargo -lmercury -labt -lssg
OBJS_PMPI = lib/libsdskv-server.so lib/libsdskv-client.so 
INCLUDE_PMPI = ../include/sdskv-common.h

INCLUDE_MAIN_GET = ../include/
FLAGS_MAIN_GET = -Wall -I$(INCLUDE_MAIN_GET) -g -o main-get
LIBS_MAIN_GET = -lmargo -lmercury -labt -lssg
OBJS_MAIN_GET = lib/libsdskv-server.so lib/libsdskv-client.so 

main-generate: main-generate.c
	$(CC) $(FLAGS_MAIN) main-generate.c $(LIBS_MAIN)

pmpi-lib:
	$(CC) $(FLAGS_PMPI) pmpi-lib.c $(OBJS_PMPI) $(LIBS_PMPI)

main-get: main-get.c
	$(CC) $(FLAGS_MAIN_GET) main-get.c $(OBJS_MAIN_GET) $(LIBS_MAIN_GET)

hello-world:
	$(CC) -Wall -g -o hello-world hello-world.c

# cannot run mpi on login node


clean:
	rm *~
