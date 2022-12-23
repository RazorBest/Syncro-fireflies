#include <iostream>
#include <omp.h>

#include "./node.h"
#include "../cells/cells.h"
#include "./communication.h"

void recv_grid_from_master(const node_state &state, simulation_area &game) {
    char buf[COMM_BUF_LEN];
    long len;

    recv_char_array(buf, &len, MASTER_RANK);
    game.grid = std::move(sendable_convert_to_area_matrix(buf, state.config.n, state.config.m));

    for (int i = 0; i < game.grid.n; i++) {
        for (int j = 0; j < game.grid.m; j++) {
            // Copy vector
            game.future_grid[i][j].v = game.grid[i][j].v;
        }

    }
}

void send_grid_to_master(const node_state &state, const simulation_area &game) {
    const node_config &config = state.config;
    char buf[COMM_BUF_LEN];
    int len;

    auto &&sub = game.grid.submatrix(config.work_x, config.work_y, config.work_n, config.work_m);
    area_matrix_view_convert_to_sendable(buf, &len, sub);
    send_char_array(buf, len, MASTER_RANK);
}

void write_in_node_data(node_state &state, const area_matrix &grid) {
    auto &data = state.data;

    for (int i = 0; i < 8; i++) {
        if (state.config.neighbour_ids[i] == -1) {
            continue;
        }
        const auto &neigh = state.config.neighbour_dst_info[i];

        auto &&sub = grid.submatrix(neigh.x, neigh.y, neigh.n, neigh.m);
        area_matrix_view_convert_to_sendable(data.send_buf[i], &data.send_size[i], sub);
    }
}

void update_grid_with_neighbours_data(area_matrix &grid, node_state &state) {
    const int *neighbour_ids = state.config.neighbour_ids;
    node_data &data = state.data;

    for (int i = 0; i < 8; i++) {
        if (state.config.neighbour_ids[i] == -1) {
            continue;
        }
        const auto &neigh = state.config.neighbour_src_info[i];

        sendable_convert_to_area_submatrix(data.recv_buf[i], grid, neigh.x,
            neigh.y, neigh.n, neigh.m);
    }
}

void update_simulation_and_swap_omp(simulation_area &game, int offx, int offy, int n, int m) {
    #pragma omp parallel for schedule(dynamic)
    for (int i = offx; i < offx + n; i++) {
        for (int j = offy; j < offy + m; j++) {
            for (int k = 0; k < game.grid[i][j].v.size(); k++) {
                game.future_grid[i][j].v[k] = game.grid[i][j].v[k];
                update_cell(game.grid, i, j, k, game.future_grid[i][j].v[k]);
            }
        }
    }
    std::swap(game.grid, game.future_grid);
}

static void update(node_state &state, simulation_area &game) {
    int x = state.config.work_x;
    int y = state.config.work_y;
    update_simulation_and_swap_omp(game, x, y, state.config.work_n, state.config.work_m);
}

static void compute(node_state &state, simulation_area &game, int cycles) {
    for (int i = 0; i < cycles; i++) {
        update(state, game);
        write_in_node_data(state, game.grid);
        communicate_with_neighbours(&state.config, &state.data);
        update_grid_with_neighbours_data(game.grid, state);
    }
}

void worker_loop(node_state &state, simulation_area &game) {
    while(1) {
        node_task task = recv_task_from_master();

        if (task.task_type == TASK_QUIT) {
            break;
        }

        if (task.task_type == TASK_COMPUTE) {
            compute(state, game, task.cycles);
            continue;
        }

        if (task.task_type == TASK_GATHER) {
            send_grid_to_master(state, game);
            continue;
        }
    }
}

void config_openmp(uint n_threads) {
    omp_set_num_threads(n_threads);
}

void run_worker(int n_threads) {
    node_state state;

    config_openmp(n_threads);

    recv_config_from_master(&state.config);

    simulation_area game(state.config.n, state.config.m);

    recv_grid_from_master(state, game);

    worker_loop(state, game);
}
