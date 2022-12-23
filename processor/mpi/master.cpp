#include <iostream>
#include <string>
#include <chrono>


#include "./node.h"
#include "../cells/cells.h"
#include "./communication.h"
#include "../server_connection/server_connection.h"

#define MAX_WORKERS 256

#define UPDATES_PER_EPOCH 1000

typedef std::chrono::steady_clock steady_clock;

void send_area_to_worker(area &a, int worker_id) {
    char buf[COMM_BUF_LEN];
    int len = 0;

    area_convert_to_sendable(buf, &len, a);
    send_char_array(buf, len * sizeof(buf), worker_id);
}

void send_grid_to_workers(const area_matrix &grid, int n_workers, const node_config *worker_configs) {
    char buf[COMM_BUF_LEN];
    int len = 0;

    for (int id = 1; id <= n_workers; id++) {
        const node_config &config = worker_configs[id];

        auto &&subarea = grid.submatrix(config.x, config.y, config.n, config.m);
        area_matrix_view_convert_to_sendable(buf, &len, subarea);
        send_char_array(buf, len, id);
    }
}

void recv_grid_from_workers(area_matrix &grid, int n_workers, const node_config *worker_configs) {
    char buf[COMM_BUF_LEN];
    long len;


    for (int id = 1; id <= n_workers; id++) {
        const node_config &config = worker_configs[id];

        recv_char_array(buf, &len, id);
        sendable_convert_to_area_submatrix(buf, grid, config.work_x, config.work_y, config.work_n, config.work_m);

    }
}

void send_task_to_workers(int task_type, int cycles, int n_workers) {
    for (int id = 1; id <= n_workers; id++) {
        send_task_to_worker(id, task_type, cycles);
    }
}

void update(simulation_area &game, int n_workers, const node_config *worker_configs) {
    send_task_to_workers(TASK_COMPUTE, UPDATES_PER_EPOCH, n_workers);

    send_task_to_workers(TASK_GATHER, 0, n_workers);
    recv_grid_from_workers(game.grid, n_workers, worker_configs);
}

void master_loop(simulation_area &game, int n_workers, const node_config *worker_configs,
        ServerConnection &connection, int n_updates) {
    int updatePerSec = 1;//120;
    int sendPerSec = 1;

    typedef std::chrono::duration<int, std::micro> microsecs;

    microsecs deltaUpdate(1000000 / updatePerSec);
    microsecs deltaSend(1000000 / sendPerSec);

    steady_clock::time_point tLast1 = steady_clock::now();
    steady_clock::time_point tLast2 = steady_clock::now();
    steady_clock::time_point tCurrent = steady_clock::now();

    int count = 0;
    while (count + UPDATES_PER_EPOCH < n_updates) {
        tCurrent = steady_clock::now();
        
        if (tCurrent - tLast1 >= deltaUpdate) {
            while (tLast1 + deltaUpdate <= tCurrent) {
                tLast1 += deltaUpdate;
            }

            update(game, n_workers, worker_configs);
            count += UPDATES_PER_EPOCH;
        }

        tCurrent = steady_clock::now();
        if (tCurrent - tLast2 >= deltaSend) {
            while (tLast2 + deltaSend <= tCurrent) {
                tLast2 += deltaSend;
            }

            connection.send_state(game.grid);
        }
    }

    if (count < n_updates) {
        send_task_to_workers(TASK_COMPUTE, n_updates - count, n_workers);

        send_task_to_workers(TASK_GATHER, 0, n_workers);
        recv_grid_from_workers(game.grid, n_workers, worker_configs);
    }
}

void run_master(const std::string &hostname, int port, int n_workers,
        int worker_lines, int worker_cols,
        int grid_n, int grid_m,
        int n_updates, bool do_store_cells) {
    if (n_workers > MAX_WORKERS) {
        printf("n_workers is to big. Limit is %d\n", MAX_WORKERS);
        return;
    }
    if (n_workers != worker_lines * worker_cols) {
        printf("Wrong configuration. nworker_lines * worker_cols must be equal with n_workers\n");
        return;
    }
    if (worker_lines > grid_n) {
        printf("Wrong configuration. worker_lines is bigger than grid_n\n");
        return;
    }
    if (worker_cols > grid_m) {
        printf("Wrong configuration. worker_cols is bigger than grid_m\n");
        return;
    }

    ServerConnection connection(hostname, port);

    connection.start_connection();
    // std::cout << "Started connection on port " << port << '\n';

    // Generate the grid
    simulation_area game(grid_n, grid_m);
    int n_cells = 100;
    generate_cells_uniform(game, n_cells, {1, 40}, {1, 40}, 13);

    int max_work_n = grid_n / worker_lines;
    if (grid_n % worker_lines) {
        max_work_n++;
    }
    int max_work_m = grid_m / worker_cols;
    if (grid_m % worker_cols) {
        max_work_m++;
    }

    // Indexing starts from 1
    node_config worker_configs[MAX_WORKERS + 1];
    int worker_id = 1;
    for (int i = 0; i < worker_lines; i++) {
        for (int j = 0; j < worker_cols; j++) {
            worker_configs[worker_id] = get_worker_grid_config(worker_id, i, j, worker_lines,
                worker_cols, grid_n, grid_m, max_work_n, max_work_m);

            node_config &&rel_config = make_config_relative_to_worker(worker_configs[worker_id]);
            send_config_to_node(worker_id, rel_config);

            worker_id++;
        }
    }

    // Send grid initial states to workers
    send_grid_to_workers(game.grid, n_workers, worker_configs);

    // Send config data to the server
    connection.send_config_data(game.grid.n, game.grid.m, n_cells);

    master_loop(game, n_workers, worker_configs, connection, n_updates);

    send_task_to_workers(TASK_QUIT, 0, n_workers);

    if (do_store_cells) {
        store_cells(game.grid, std::cout);
    }
}
