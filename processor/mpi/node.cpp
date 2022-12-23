#include "mpi.h"
#include "./node.h"

node_info get_worker_grid_info(int worker_x, int worker_y, int n, int m, int max_work_n, int max_work_m) {
    node_info info;

    int work_x = worker_x * max_work_n;
    int work_y = worker_y * max_work_m;
    info.x = work_x;
    info.y = work_y;

    int work_n = std::min(work_x + max_work_n, n) - work_x;
    int work_m = std::min(work_y + max_work_m, m) - work_y;
    info.n = work_n;
    info.m = work_m;

    return info;
}

node_info get_bordered_area(node_info info, int n, int m) {
    node_info bordered_info;

    bordered_info.x = std::max(info.x - 1, 0);
    bordered_info.y = std::max(info.y - 1, 0);

    bordered_info.n = std::min(info.x + info.n + 1, n) - bordered_info.x;
    bordered_info.m = std::min(info.y + info.m + 1, m) - bordered_info.y;

    return bordered_info;
}

node_config get_worker_grid_config(int worker_id, int worker_x, int worker_y, int worker_lines, int worker_cols,
        int n, int m, int max_work_n, int max_work_m) {
    // Worker ids start from 1
    node_config config;

    if (worker_x * max_work_n >= n + max_work_n) {
        std::cout << "Unhandled grid config\n";
        return config; 
    }
    if (worker_y * max_work_m >= m + max_work_m) {
        std::cout << "Unhandled grid config\n";
        return config;
    }

    node_info own_info = get_worker_grid_info(worker_x, worker_y, n, m, max_work_n, max_work_m);

    config.rank = worker_id;

    int work_x = own_info.x;
    int work_y = own_info.y;
    config.work_x = work_x;
    config.work_y = work_y;

    int work_n = own_info.n;
    int work_m = own_info.m;
    config.work_n = work_n;
    config.work_m = work_m;

    int x = std::max(work_x - 1, 0);
    int y = std::max(work_y - 1, 0);
    config.x = x;
    config.y = y;

    config.n = std::min(work_x + work_n + 1, n) - x;
    config.m = std::min(work_y + work_m + 1, m) - y;

    int dir[8][2] = {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1},
        {-1, 0}, {-1, 1}};
    for (int k = 0; k < 8; k++) {
        int neigh_worker_x = worker_x + dir[k][0], neigh_worker_y = worker_y + dir[k][1];
        // Get work area of neighbour
        node_info info = get_worker_grid_info(neigh_worker_x, neigh_worker_y,
            n, m, max_work_n, max_work_n);
        
        config.neighbour_ids[k] = -1;

        if (info.x < 0 || info.x >= n) {
            continue;
        }
        if (info.y < 0 || info.y >= m) {
            continue;
        }

        config.neighbour_ids[k] = neigh_worker_x * worker_cols + neigh_worker_y + 1;

        // compute intersection
        node_info intersection = get_intersection(info, {config.x, config.y, config.n, config.m});
        // Translate to current node x and y
        intersection.x -= config.x;
        intersection.y -= config.y;
        config.neighbour_src_info[k] = intersection;

        info = get_bordered_area(info, n, m);
        intersection = get_intersection(own_info, info);
        // Translate to current node x and y
        intersection.x -= config.x;
        intersection.y -= config.y;
        config.neighbour_dst_info[k] = intersection;
    }

    return config;
}

node_config make_config_relative_to_worker(const node_config &config) {
    node_config rel_config = config;

    rel_config.work_x -= config.x;
    rel_config.work_y -= config.y;

    return rel_config;
}

void send_config_to_node(int worker_id, node_config &config) {
    // Send the neighbour ids
    MPI_Send(&config, sizeof(config), MPI_CHAR, worker_id, 0, MPI_COMM_WORLD);
}

/*
void send_grid_config_to_worker_v1(int worker_id, int n, int m, int max_work_n, int max_work_m) {
    // Worker ids start from 1

    node_config config;

    config.rank = worker_id;

    int x = (worker_id - 1) / m;
    int y = (worker_id - 1) % m;
    config.x = x;
    config.y = y;

    config.n = std::min(x - 1 + 3U, (unsigned int)n) - std::max(x - 1, 0);
    config.m = std::min(y - 1 + 3U, (unsigned int)m) - std::max(y - 1, 0);

    config.work_x = x - std::max(x - 1, 0);
    config.work_y = y - std::max(y - 1, 0);

    int dir[8][2] = {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1},
        {-1, 0}, {-1, 1}};
    for (int k = 0; k < 8; k++) {
        int nx = config.x + dir[k][0], ny = config.y + dir[k][1];

        config.neighbours[k] = -1;

        if (nx < 0 || nx >= n) {
            continue;
        }
        if (ny < 0 || ny >= m) {
            continue;
        }
        
        config.neighbours[k] = nx * m + ny + 1;
    }

    // Send the neighbour ids
    MPI_Send(&config, sizeof(config), MPI_CHAR, worker_id, 0, MPI_COMM_WORLD);
}
*/

void recv_config_from_master(node_config *config) {
    MPI_Recv(config, sizeof(*config), MPI_CHAR, MASTER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

void send_task_to_worker(int worker_id, int task_type, int cycles) {
    node_task task;

    task.task_type = task_type;
    task.cycles = cycles;

    MPI_Send(&task, sizeof(task), MPI_CHAR, worker_id, 0, MPI_COMM_WORLD);
}

node_task recv_task_from_master() {
    node_task task;
    MPI_Recv(&task, sizeof(task), MPI_CHAR, MASTER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    return task;
}

void communicate_with_neighbours(const node_config *config, node_data *data) {
    MPI_Request handlers[8];

    for (int i = 0; i < 8; i++) {
        int neigh_id = config->neighbour_ids[i];
        if (neigh_id == -1) {
            continue;
        }

        MPI_Irecv(&data->recv_size[i], 1, MPI_INT, neigh_id, 0, MPI_COMM_WORLD, &handlers[i]);
        MPI_Send(&data->send_size[i], 1, MPI_INT, neigh_id, 0, MPI_COMM_WORLD);
    }

    for (int i = 0; i < 8; i++) {
        int neigh_id = config->neighbour_ids[i];
        if (neigh_id == -1) {
            continue;
        }

        MPI_Status status;
        MPI_Wait(&handlers[i], &status);
    }


    for (int i = 0; i < 8; i++) {
        int neigh_id = config->neighbour_ids[i];
        if (neigh_id == -1) {
            continue;
        }

        MPI_Irecv(data->recv_buf[i], data->recv_size[i], MPI_CHAR, neigh_id, 0, MPI_COMM_WORLD, &handlers[i]);
        MPI_Send(data->send_buf[i], data->send_size[i], MPI_CHAR, neigh_id, 0, MPI_COMM_WORLD);
    }

    for (int i = 0; i < 8; i++) {
        int neigh_id = config->neighbour_ids[i];
        if (neigh_id == -1) {
            continue;
        }

        MPI_Status status;
        MPI_Wait(&handlers[i], &status);
    }
}

node_info get_intersection(node_info src, node_info dst) {
    int x1 = src.x, y1 = src.y;
    int n1 = src.n, m1 = src.m;

    int x2 = dst.x, y2 = dst.y;
    int n2 = dst.n, m2 = dst.m;

    int diff1 = x1 + n1 - x2;
    int diff2 = x2 + n2 - x1;

    int xi = 0, yi = 0, ni, mi;

    node_info intersection = {0, 0, 0, 0};

    if (diff1 <= 0 || diff2 <= 0) {
        return intersection;
    }

    if (x2 >= x1) {
        xi = x2;
    } else {
        xi = x1;
    }

    if (x2 + n2 <= x1 + n1) {
        ni = x2 + n2 - xi;
    } else {
        ni = x1 + n1 - xi;
    }

    diff1 = y1 + m1 - y2;
    diff2 = y2 + m2 - y1;
    if (diff1 <= 0 || diff2 <= 0) {
        return intersection;
    }

    if (y2 >= y1) {
        yi = y2;
    } else {
        yi = y1;
    }

    if (y2 + m2 <= y1 + m1) {
        mi = y2 + m2 - yi;
    } else {
        mi = y1 + m1 - yi;
    }

    intersection.x = xi;
    intersection.y = yi;
    intersection.n = ni;
    intersection.m = mi;

    return intersection;
}
