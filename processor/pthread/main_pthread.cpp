#include <iostream>
#include <climits>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <chrono>
#include <ratio>
#include <pthread.h>

#include "../cells/cells.h"
#include "../server_connection/server_connection.h"

#define LIGHT_OFF 0
#define LIGHT_ON 1
#define T_MAX 900
#define T_MIN 30

// Distance from which 2 cells are considered
// neighbours
#define MAX_DIST 5

#define MAX_THREADS 128

#define UPDATES_PER_EPOCH 1000

typedef unsigned int uint;
typedef std::chrono::steady_clock steady_clock;

bool THREADS_RUNNING = true;
pthread_barrier_t barrier;
pthread_barrier_t worker_barrier;
pthread_mutex_t mutex;
int N_CYCLES = 0;

struct thread_config {
    int tid;
    simulation_area *game;
    int work_x, work_y;
    int work_n, work_m;
};

void* worker_thread(void *args) {
    thread_config *config = (thread_config*)args;

    int tid = config->tid;
    simulation_area *game = config->game;
    int work_x = config->work_x;
    int work_y = config->work_y;
    int work_n = config->work_n;
    int work_m = config->work_m;

    while (THREADS_RUNNING) {
        pthread_barrier_wait(&barrier);
        for (int i = 0; i < N_CYCLES; i++) {
            // The actual work is done here
            update_simulation(*game, work_x, work_y, work_n, work_m);
            pthread_barrier_wait(&worker_barrier);
            if (tid == 0) {
                std::swap(game->grid, game->future_grid);
            }
            pthread_barrier_wait(&worker_barrier);
        }
        pthread_barrier_wait(&barrier);
    }


    return NULL;
}

void set_task(int cycles) {
    N_CYCLES = cycles;
}

void update(simulation_area &game, int cycles) {
    set_task(cycles);
    pthread_barrier_wait(&barrier);
    pthread_barrier_wait(&barrier);
}

void main_loop(simulation_area &game, ServerConnection &connection, int n_updates) {
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

            update(game, UPDATES_PER_EPOCH);
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
        update(game, n_updates - count);
    }
}

void start_threads(pthread_t *threads, thread_config *configs, int n_threads,
        simulation_area &game,
        int n, int m,
        int worker_lines, int worker_cols,
        int max_work_n, int max_work_m) {
    
    if (worker_lines * worker_cols != n_threads) {
        printf("Invalid config for threads\n");
        exit(-1);
    }

    for (int i = 0; i < n_threads; i++) {
        configs[i].tid = i;
        configs[i].game = &game;

        int worker_x = i / worker_cols;
        int worker_y = i % worker_cols;

        int work_x = worker_x * max_work_n;
        int work_y = worker_y * max_work_m;
        configs[i].work_x = work_x;
        configs[i].work_y = work_y;

        int work_n = std::min(work_x + max_work_n, n) - work_x;
        int work_m = std::min(work_y + max_work_m, m) - work_y;
        configs[i].work_n = work_n;
        configs[i].work_m = work_m;

        int ret = pthread_create(&threads[i], NULL, worker_thread, &configs[i]);
        if (ret) {
            printf("Thread creation error\n");
            exit(-1);
        }
    }
}

void stop_threads(pthread_t *threads, int n_threads) {
    for (int i = 0; i < n_threads; i++) {
        int ret = pthread_join(threads[i], NULL);
        if (ret) {
            printf("Thread join error\n");
            exit(-1);
        }
    }
}

int main(int argc, char const *argv[])
{
    int grid_n, grid_m;
    int n_updates = INT_MAX;
    bool do_store_cells = false;
    int n_threads;
    int worker_lines, worker_cols;

    if (argc < 8) {
        printf("Usage: %s n_threads worker_lines worker_cols grid_n grid_m n_updates do_store_cells\n", argv[0]);
        return 0;
    }

    n_threads = atoi(argv[1]);
    if (n_threads > MAX_THREADS) {
        printf("Max threads limit exceeded\n");
        return -1;
    }
    worker_lines = atoi(argv[2]);
    worker_cols = atoi(argv[3]);
    grid_n = atoi(argv[4]);
    grid_m = atoi(argv[5]);
    n_updates = atoi(argv[6]);
    if (n_updates < 0) {
        n_updates = INT_MAX;
    }
    do_store_cells = atoi(argv[7]);


	std::string hostname{"127.0.0.1"};
	uint16_t port = 18273;

    int n_cells = 100;
    simulation_area game(grid_n, grid_m);
    // generate_cells_uniform(game, n_cells, {10, 100}, {10, 100}, 13);
    generate_cells_uniform(game, n_cells, {1, 40}, {1, 40}, 13);

    ServerConnection connection(hostname, port);
    connection.start_connection();
    connection.send_config_data(game.grid.n, game.grid.m, n_cells);

    int max_work_n = grid_n / worker_lines;
    if (grid_n % worker_lines) {
        max_work_n++;
    }
    int max_work_m = grid_m / worker_cols;
    if (grid_m % worker_cols) {
        max_work_m++;
    }

    pthread_t threads[MAX_THREADS];
    thread_config thread_configs[MAX_THREADS];

    pthread_barrier_init(&barrier, NULL, n_threads + 1);
    pthread_barrier_init(&worker_barrier, NULL, n_threads);
    pthread_mutex_init(&mutex, NULL);

    start_threads(threads, thread_configs, n_threads, 
        game,
        game.grid.n, game.grid.m,
        worker_lines, worker_cols,
        max_work_n, max_work_m);


    main_loop(game, connection, n_updates);
    if (do_store_cells) {
        store_cells(game.grid, std::cout);
    }

    set_task(0);
    pthread_barrier_wait(&barrier);
    THREADS_RUNNING = false;
    pthread_barrier_wait(&barrier);

    stop_threads(threads, n_threads);

    pthread_barrier_destroy(&worker_barrier);
    pthread_barrier_destroy(&barrier);

	return 0;
}
