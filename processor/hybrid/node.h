#ifndef NODE_H
#define NODE_H

#include "./communication.h"

#define MASTER_RANK 0

#define TASK_QUIT 15
#define TASK_COMPUTE 16
#define TASK_GATHER 17

#define NEIGHBOURS_MAX 256

struct node_info {
    int x, y;
    int n, m;
};

struct node_config {
    int rank;
    int x, y;
    int n, m;
    int work_x, work_y;
    int work_n, work_m;
    int neighbour_ids[8];
    node_info neighbour_src_info[8];
    node_info neighbour_dst_info[8];
};

struct node_data {
    int send_size[8];
    char send_buf[8][COMM_BUF_LEN]; 
    int recv_size[8];
    char recv_buf[8][COMM_BUF_LEN];
};

struct node_state {
    node_config config;
    node_data data;
};

struct node_task {
    int task_type;
    int cycles;
};

node_config get_worker_grid_config(int worker_id, int worker_x, int worker_y, int worker_lines, int worker_cols,
        int n, int m, int max_work_n, int max_work_m);
node_config make_config_relative_to_worker(const node_config &config);
void send_config_to_node(int worker_id, node_config &config);
void recv_config_from_master(node_config *config);
void send_task_to_worker(int worker_id, int task_type, int cycles);
node_task recv_task_from_master();
void communicate_with_neighbours(const node_config *config, node_data *data);

node_info get_intersection(node_info src, node_info dst);

#endif // NODE_H
