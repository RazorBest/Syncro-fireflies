#ifndef MASTER_H
#define MASTER_H

void run_master(const std::string &hostname, int port, int n_workers,
        int worker_lines, int worker_cols,
        int grid_n, int grid_m,
        int n_updates, bool do_store_cells);

#endif
