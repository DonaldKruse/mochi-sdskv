CC = mpicc
BIN = ./bin
LIB = ./lib

FLAGS_MAIN = -Wall -g -o $(BIN)/main-generate
LIBS_MAIN = -lmargo -lmercury -labt

FLAGS_PMPI = -Wall -g -shared -o $(BIN)/pmpi-lib.so 
LIBS_PMPI = -lmargo -lmercury -labt -lssg
OBJS_PMPI = $(LIB)/libsdskv-server.so $(LIB)/libsdskv-client.so $(LIB)/pmpi-common.o
INCLUDE_PMPI = ../include/sdskv-common.h pmpi-common.h

INCLUDE_MAIN_GET = ../include/
FLAGS_MAIN_GET = -Wall -g -I$(INCLUDE_MAIN_GET) -o $(BIN)/main-get
LIBS_MAIN_GET = -lmargo -lmercury -labt -lssg
OBJS_MAIN_GET = $(LIB)/libsdskv-server.so $(LIB)/libsdskv-client.so 

all: main-generate pmpi-lib pmpi-common main-get
.PHONY: all


main-generate: main-generate.c
	$(CC) $(FLAGS_MAIN) main-generate.c $(LIBS_MAIN)

pmpi-lib: pmpi-lib.c pmpi-common.o 
	$(CC) $(FLAGS_PMPI) pmpi-lib.c $(OBJS_PMPI) $(LIBS_PMPI)

pmpi-common: pmpi-common.c pmpi-common.h
	$(CC) -Wall -g -c -o $(LIB)/pmpi-common.o pmpi-common.c $(LIBS_PMPI)

main-get: main-get.c
	$(CC) $(FLAGS_MAIN_GET) main-get.c $(OBJS_MAIN_GET) $(LIBS_MAIN_GET)

hello-world:
	$(CC) -Wall -g -o $(BIN)/hello-world hello-world.c

# cannot run mpi on login node


clean:
	rm *~ *.o
