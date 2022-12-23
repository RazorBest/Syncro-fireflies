#include <iostream>
#include <climits>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <chrono>
#include <ratio>

#include "../cells/cells.h"
#include "../server_connection/server_connection.h"

#define LIGHT_OFF 0
#define LIGHT_ON 1
#define T_MAX 900
#define T_MIN 30

// Distance from which 2 cells are considered
// neighbours
#define MAX_DIST 5

typedef unsigned int uint;
typedef std::chrono::steady_clock steady_clock;


int count_neighbours(area_matrix &grid, int i, int j, int k,
        point_data &cell) {
    int neigh_count = 0;
    const int pairs[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1},
        {-1, 0}, {-1, -1}, {0, -1}, {1, -1}
    };

    // The cells in the same grid square are considered neighbours
    neigh_count = grid[i][j].v.size() - 1;

    // The other neighbours must be at the near squares
    for (int index = 0; index < 8; index++) {
        int di = pairs[index][0];
        int dj = pairs[index][1];

        // Boundaries
        if (i + di < 0 || i + di >= grid.n) {
            continue;
        }
        if (j + dj < 0 || j + dj >= grid.m) {
            continue;
        }

        const auto &v = grid[i + di][j + dj].v;

        for (const auto &c2 : v) {
            if(!cell.is_neighbour(c2)) {
                continue;
            }

            neigh_count++;
        }
    }

    return neigh_count;
}

/*
const point_data &get_nth_neighbour(area_matrix &grid, int i, int j,
        int k, const point_data &cell, int n) {
    const int pairs[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1},
        {-1, 0}, {-1, -1}, {0, -1}, {1, -1}
    };
    
    if (n < k) {
        return grid[i][j].v[n];
    }

    if (n < grid[i][j].v.size() - 1) {
        return grid[i][j].v[n + 1];
    }

    n -= grid[i][j].v.size() - 1;

    // The other neighbours must be at the near squares
    for (int index = 0; index < 8; index++) {
        int di = pairs[index][0];
        int dj = pairs[index][1];

        // Boundaries
        if (i + di < 0 || i + di >= grid.n) {
            continue;
        }
        if (j + dj < 0 || j + dj >= grid.m) {
            continue;
        }

        const auto &v = grid[i + di][j + dj].v;

        for (const auto &c2 : v) {
            if(!cell.is_neighbour(c2)) {
                continue;
            }

            // Found neighbour
            if (n == 0) {
                return c2;
            }

            n--;
        }
    }

    std::cout << "Got here. BAD ZONE!\n";
    return {};
}
*/

void main_loop(simulation_area &game, ServerConnection &connection, int n_updates) {
    int updatePerSec = 1000;//120;
    int sendPerSec = 1;

    typedef std::chrono::duration<int, std::micro> microsecs;

    microsecs deltaUpdate(1000000 / updatePerSec);
    microsecs deltaSend(1000000 / sendPerSec);

    steady_clock::time_point tLast1 = steady_clock::now();
    steady_clock::time_point tLast2 = steady_clock::now();
    steady_clock::time_point tCurrent = steady_clock::now();

    int count = 0;
    while (count < n_updates) {
        tCurrent = steady_clock::now();
        
        if (tCurrent - tLast1 >= deltaUpdate) {
            while (tLast1 + deltaUpdate <= tCurrent) {
                tLast1 += deltaUpdate;
            }

            update_simulation_and_swap(game, 0, 0, game.grid.n, game.grid.m);
            count++;
        }

        tCurrent = steady_clock::now();
        if (tCurrent - tLast2 >= deltaSend) {
            while (tLast2 + deltaSend <= tCurrent) {
                tLast2 += deltaSend;
            }

            connection.send_state(game.grid);
        }
    }
}

int main(int argc, char const *argv[])
{
    int grid_n, grid_m;
    int n_updates = INT_MAX;
    bool do_store_cells = false;

    if (argc < 5) {
        printf("Usage: %s grid_n grid_m n_updates do_store_cells\n", argv[0]);
        return 0;
    }

    grid_n = atoi(argv[1]);
    grid_m = atoi(argv[2]);
    n_updates = atoi(argv[3]);
    if (n_updates < 0) {
        n_updates = INT_MAX;
    }
    do_store_cells = atoi(argv[4]);


	std::string hostname{"127.0.0.1"};
	uint16_t port = 18273;

    int n_cells = 100;
    simulation_area game(grid_n, grid_m);
    // generate_cells_uniform(game, n_cells, {10, 100}, {10, 100}, 13);
    generate_cells_uniform(game, n_cells, {1, 40}, {1, 40}, 13);

    ServerConnection connection(hostname, port);
    connection.start_connection();
    connection.send_config_data(game.grid.n, game.grid.m, n_cells);

    main_loop(game, connection, n_updates);
    if (do_store_cells) {
        store_cells(game.grid, std::cout);
    }

	return 0;
}
