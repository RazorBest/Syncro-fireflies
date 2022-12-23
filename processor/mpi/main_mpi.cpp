#include <iostream>
#include <climits>
#include "mpi.h"

#include "./master.h"
#include "./worker.h"

void config_mpi(int argc, char *argv[],
        int *cluster_size, int *rank) {
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, cluster_size);
    MPI_Comm_rank(MPI_COMM_WORLD, rank);
}

int main(int argc, char *argv[]) {
	std::string hostname{"127.0.0.1"};
	uint16_t port = 18273;

    int n_updates = INT_MAX;
    bool do_store_cells = false;
    int cluster_size, rank;
    int worker_lines = 0, worker_cols = 0;
    int grid_n = 0, grid_m = 0;

    config_mpi(argc, argv, &cluster_size, &rank);

    if (argc < 7) {
        if (rank == 0) {
            printf("Usage: %s worker_lines worker_cols grid_n grid_m n_updates do_store_cells\n", argv[0]);
        }

        MPI_Finalize();
        return 0;
    }

    worker_lines = atoi(argv[1]);
    worker_cols = atoi(argv[2]);
    grid_n = atoi(argv[3]);
    grid_m = atoi(argv[4]);
    n_updates = atoi(argv[5]);
    if (n_updates < 0) {
        n_updates = INT_MAX;
    }
    do_store_cells = atoi(argv[6]);

    if (rank == 0) {
        run_master(hostname, port, cluster_size - 1, worker_lines,
            worker_cols, grid_n, grid_m, n_updates, do_store_cells);
    } else {
        run_worker();
    }

    MPI_Finalize();

	return 0;
}
