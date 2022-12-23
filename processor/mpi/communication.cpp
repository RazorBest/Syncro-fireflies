#include "./communication.h"
#include "mpi.h"

void send_char_array(char *array, long size, int dest) {
    MPI_Send(&size, 1, MPI_LONG, dest, 0, MPI_COMM_WORLD);
    MPI_Send(array, size, MPI_CHAR, dest, 0, MPI_COMM_WORLD);
}

void recv_char_array(char *array, long *size, int source) {
    MPI_Recv(size, 1, MPI_LONG, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(array, *size, MPI_CHAR, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
