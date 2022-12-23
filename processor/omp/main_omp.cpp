#include <iostream>
#include <climits>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <chrono>
#include <ratio>
#include <omp.h>

#include <arpa/inet.h> // htons, inet_addr
#include <netinet/in.h> // sockaddr_in
#include <sys/types.h> // uint16_t
#include <sys/socket.h> // socket, sendto
#include <unistd.h> // close

// The max number of areas in a grid will be S_MAX*S_MAX
#define S_MAX 4

#define LIGHT_OFF 0
#define LIGHT_ON 1
#define T_MAX 900
#define T_MIN 30

// Distance from which 2 cells are considered
// neighbours
#define MAX_DIST 5

#define BUF_LEN 1024

typedef unsigned int uint;
typedef std::chrono::steady_clock steady_clock;

// Returns a random floating point number x
// such that lo <= x <= hi
float rand_float(float lo, float hi) {
    return lo + (rand()) * (hi - lo) / (float)RAND_MAX;
}

// Returns a random integer x
// such that lo <= x <= hi
int rand_int(int lo, int hi) {
    return lo + (rand() % (hi - lo + 1));
}


struct point_data {
    uint id;
    // time off
    int t1;
    // time on
    int t2;
    float x;
    float y;
    char light;
    int cycle;
    int counter;

    bool is_neighbour(const point_data &p) const {
        return (x-p.x)*(x-p.x) + (y-p.y)*(y-p.y) <= MAX_DIST*MAX_DIST;
    }

    void adapt_to_1(const point_data &p) {
        if (light == LIGHT_OFF && p.light == LIGHT_ON) {
            if (t1 < T_MAX) {
                t1++;
            }
            if (t2 > T_MIN) {
                t2--;
            }
            //t1 = rand_int(T_MIN, T_MAX);
            //t2 = rand_int(T_MIN, T_MAX);

        } else if (light == LIGHT_ON && p.light == LIGHT_OFF) {
            if (t1 > T_MIN) {
                t1--;
            }
            if (t2 < T_MAX) {
                t2++;
            }
            //t1 = rand_int(T_MIN, T_MAX);
            //t2 = rand_int(T_MIN, T_MAX);
        }
    }

    void adapt_to_2(const point_data &p) {
        if (t1 < p.t1) {
            t1++;
        } else if (t1 > p.t1) {
            t1--;
        }
        if (t2 > p.t2) {
            t2--;
        // There's a bug here (p.t1)
        } else if (t2 < p.t1) {
            t2++;
        }
    }

    void adapt_to_3(const point_data &p) {
        if (t1 < p.t1) {
            t1++;
        } else if (t1 > p.t1) {
            t1--;
        }
        if (t2 > p.t2) {
            t2--;
        } else if (t2 < p.t2) {
            t2++;
        }
    }

    void adapt_to_4(const point_data &p) {
        if (t1 < p.t1) {
            t1++;
        } else if (t1 > p.t1) {
            t1--;
        }
        if (t2 > p.t2) {
            t2--;
        } else if (t2 < p.t2) {
            t2++;
        }
        if (counter != p.counter) {
            counter++;
        }
    }

    void adapt_to(const point_data &p) {
        if (t1 < p.t1) {
            t1++;
        } else if (t1 > p.t1) {
            t1--;
        }
        if (t2 > p.t2) {
            t2--;
        } else if (t2 < p.t2) {
            t2++;
        }
        if (counter > p.counter) {
            counter--;
        } else if (counter < p.counter) {
            counter++;
        }
    }

    void adapt_to_given(const std::vector<point_data> v, int k,
            std::vector<const point_data*> far_neigh) {
        int same = 0;
        int different = 0;

        // Iterate through the neighbours in the same grid square
        for (int i = 0; i < k; i++) {
            if (light != v[i].light) {
                different++;
                continue;
            }

            same++;
        }
        for (int i = k + 1; i < v.size(); i++) {
            if (light != v[i].light) {
                different++;
                continue;
            }

            same++;
        }

        for (const point_data* neigh : far_neigh) {
            if (light != neigh->light) {
                different++;
                continue;
            }

            same++;
        }

        if (same > different) {
            return;
        }

        // Choose a neighbour depending on the cycle
        const point_data *neighbour;

        if (cycle < k) {
            neighbour = &v[cycle];
        } else if (cycle < v.size() - 1) {
            neighbour = &v[cycle + 1];
        } else {
            neighbour = far_neigh[cycle - v.size() + 1];
        }

        if (t1 < neighbour->t1) {
            t1++;
        } else if (t1 > neighbour->t1) {
            t1--;
        }
        if (t2 > neighbour->t2) {
            t2--;
        } else if (t2 < neighbour->t2) {
            t2++;
        }
        if (counter > neighbour->counter) {
            counter--;
        } else if (counter < neighbour->counter - 1) {
            counter += 2;
        } else if (counter < neighbour->counter) {
            counter++;
        }

    }
};

struct sendable_point_data {
	uint id;
	float x, y;
	int t1, t2;
	char counter;
};

struct area {
    std::vector<point_data> v;
};

struct area_matrix {
    area _grid1[S_MAX][S_MAX];
    area _grid2[S_MAX][S_MAX];
    area (*grid)[S_MAX];
    area (*future_grid)[S_MAX];
    // Each area will cover a 10x10 square
    // For this algorithm, it should be  between
    // 2/3 * l and l*sqrt(2)
    // Where l is MAX_DIST
    float area_length = 10;
    uint n;
};

// Generate n cells
void generate_cells_uniform(area_matrix &grid, int n, 
        const std::pair<float, float> &x_limits,
        const std::pair<float, float> &y_limits,
        unsigned int seed) {
    srand(seed);
    if (x_limits.first > x_limits.second || 
            y_limits.first > y_limits.second) {
        std::cout << "Generate cells: error in limits\n";
        return;
    }

    float sq_l = MAX_DIST / sqrt(2) - 0.5;

    grid.n = 0;

    uint index = 0;

    while (n) {
        for (float x = x_limits.first; x < x_limits.second && n; x += sq_l) {
            for (float y = y_limits.first; y < y_limits.second && n; y += sq_l) {
                int t1 = rand_int(T_MIN, T_MAX);
                int t2 = rand_int(T_MIN, T_MAX);

                int area_i = floor(x / grid.area_length);
                int area_j = floor(y / grid.area_length);

                if (area_i < 0 || area_j < 0 || area_i >= S_MAX || area_j >= S_MAX) {
                    std::cout << "Generate cells: area out of bounds\n";
                    continue;
                }

                point_data cell{index, t1, t2, x, y, LIGHT_ON, 0};
                grid.grid[area_i][area_j].v.push_back(cell);
                grid.future_grid[area_i][area_j].v.push_back(cell);
                grid.n++;

                n--;
                index++;
            }
        }
    }
}

// Generate n cells
void generate_cells(area_matrix &grid, int n, 
        const std::pair<float, float> &x_limits,
        const std::pair<float, float> &y_limits,
        unsigned int seed) {
    srand(seed);

    if (x_limits.first > x_limits.second || 
            y_limits.first > y_limits.second) {
        std::cout << "Generate cells: error in limits\n";
        return;
    }

    grid.n = 0;

    for (uint i = 0; i < n; i++) {
        float x = rand_float(x_limits.first, x_limits.second);
        float y = rand_float(y_limits.first, y_limits.second);
        int t1 = rand_int(T_MIN, T_MAX);
        int t2 = rand_int(T_MIN, T_MAX);

        int area_i = floor(x / grid.area_length);
        int area_j = floor(y / grid.area_length);

        if (area_i < 0 || area_j < 0 || area_i >= S_MAX || area_j >= S_MAX) {
            std::cout << "Generate cells: area out of bounds\n";
            continue;
        }

        point_data cell{i, t1, t2, x, y, LIGHT_ON, 0};
        grid.grid[area_i][area_j].v.push_back(cell);
        grid.future_grid[area_i][area_j].v.push_back(cell);
        grid.n++;
    }
}

void store_cells(const area_matrix &grid, std::ostream &out) {
    out << grid.n << '\n';
    for (int i = 0; i < S_MAX; i++) {
        for (int j = 0; j < S_MAX; j++) {
            for (const auto &cell : grid.grid[i][j].v) {
                    out << cell.t1 << ' ' << cell.t2 << ' '; 
                    out << cell.counter << ' '; 
                    out << cell.x << ' ' << cell.y << ' '; 
                    out << unsigned(cell.light) << '\n';
                }
        }
    }
}

int count_neighbours(area_matrix &g, int i, int j, int k,
        point_data &cell) {
    auto &grid = g.grid;
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
        if (i + di < 0 || i + di >= S_MAX) {
            continue;
        }
        if (j + dj < 0 || j + dj >= S_MAX) {
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
const point_data &get_nth_neighbour(area_matrix &g, int i, int j,
        int k, const point_data &cell, int n) {
    auto &grid = g.grid;
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
        if (i + di < 0 || i + di >= S_MAX) {
            continue;
        }
        if (j + dj < 0 || j + dj >= S_MAX) {
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

std::vector<const point_data*> get_far_neighbours(area_matrix &g, int i, int j,
        int k, const point_data &cell) {
    auto &grid = g.grid;
    const int pairs[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1},
        {-1, 0}, {-1, -1}, {0, -1}, {1, -1}
    };
    const int p_len = 8;

    std::vector<const point_data*> neighbours;

    // The other neighbours must be at the near squares
    for (int index = 0; index < p_len; index++) {
        int di = pairs[index][0];
        int dj = pairs[index][1];

        // Boundaries
        if (i + di < 0 || i + di >= S_MAX) {
            continue;
        }
        if (j + dj < 0 || j + dj >= S_MAX) {
            continue;
        }

        const auto &v = grid[i + di][j + dj].v;

        for (auto it = v.begin(); it != v.end(); it++) {
            if(!cell.is_neighbour(*it)) {
                continue;
            }

            // Add the address of the element
            //  in the vector
            neighbours.push_back(&(*it));
        }
    }

    return neighbours;
}

void update_cell(area_matrix &g, int i, int j, int k,
        point_data &cell) {
    auto &grid = g.grid;
    auto light = cell.light;
    auto cycle = cell.cycle;
    int index = -1; 

    auto far_neighbours = get_far_neighbours(g, i, j, k, cell);
    int neigh_count = grid[i][j].v.size() - 1 + far_neighbours.size();
    if (cell.cycle >= neigh_count) {
        cell.cycle = 0;
    }
    if (neigh_count > 0) {
        cell.adapt_to_given(grid[i][j].v, k, far_neighbours);
    }

    cell.cycle++;

    // You may need to increment more here
    if (!cell.light) {
        if (cell.counter >= cell.t1) {
            cell.counter = 0;
            cell.light = 1;
        }
    } else {
        if (cell.counter >= cell.t2) {
            cell.counter = 0;
            cell.light = 0;
        }
    }
    cell.counter++;
}

void update(area_matrix &grid) {
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < S_MAX; i++) {
        for (int j = 0; j < S_MAX; j++) {
            for (int k = 0; k < grid.grid[i][j].v.size(); k++) {
                grid.future_grid[i][j].v[k] = grid.grid[i][j].v[k];
                update_cell(grid, i, j, k, grid.future_grid[i][j].v[k]);
            }
        }
    }
    std::swap(grid.grid, grid.future_grid);
}

void send_state(area_matrix &grid, int sock, sockaddr_in destination) {
	int n_bytes;
    sendable_point_data buf[BUF_LEN];
    int len = 0;

    for (int i = 0; i < S_MAX; i++) {
        for (int j = 0; j < S_MAX; j++) {
            for (int k = 0; k < grid.grid[i][j].v.size(); k++) {
                auto &cell = grid.grid[i][j].v[k];

                buf[len].id = cell.id;
                buf[len].x = cell.x;
                buf[len].y = cell.y;
                buf[len].t1 = cell.t1;
                buf[len].t2 = cell.t2;
                buf[len].counter = cell.counter;
                len++;

                if (len >= BUF_LEN) {
                    n_bytes = ::sendto(sock, buf, sizeof(*buf) * BUF_LEN, 0,
                            reinterpret_cast<sockaddr*>(&destination), 
                            sizeof(destination)
                    );

                    len = 0;
                }
            }
        }
    }

    n_bytes = ::sendto(sock, buf, sizeof(*buf) * len, 0,
            reinterpret_cast<sockaddr*>(&destination), 
            sizeof(destination)
    );

}

void main_loop(area_matrix &grid, int sock, sockaddr_in destination) {
    int updatePerSec = 1000;//120;
    int sendPerSec = 1;

    typedef std::chrono::duration<int, std::micro> microsecs;

    microsecs deltaUpdate(1000000 / updatePerSec);
    microsecs deltaSend(1000000 / sendPerSec);

    steady_clock::time_point tLast1 = steady_clock::now();
    steady_clock::time_point tLast2 = steady_clock::now();
    steady_clock::time_point tCurrent = steady_clock::now();

    int count = 0;
    while (count < 500) {
        tCurrent = steady_clock::now();
        
        if (tCurrent - tLast1 >= deltaUpdate) {
            while (tLast1 + deltaUpdate <= tCurrent) {
                tLast1 += deltaUpdate;
            }

            update(grid);
            count++;
        }

        tCurrent = steady_clock::now();
        if (tCurrent - tLast2 >= deltaSend) {
            while (tLast2 + deltaSend <= tCurrent) {
                tLast2 += deltaSend;
            }

            send_state(grid, sock, destination);
        }
    }
}

void config_openmp(uint n_threads) {
    omp_set_num_threads(n_threads);
}


int main(int argc, char const *argv[])
{
	std::string hostname{"127.0.0.1"};
	uint16_t port = 18273;

    if (argc < 6) {
        printf("Usage: %s n_threads grid_n grid_m n_updates do_store_cells\n", argv[0]);
        return 0;
    }

    int n_threads = atoi(argv[1]);
    int grid_n = atoi(argv[2]);
    int grid_m = atoi(argv[3]);
    int n_updates = atoi(argv[4]);
    if (n_updates < 0) {
        n_updates = INT_MAX;
    }
    int do_store_cells = atoi(argv[5]);

    config_openmp(n_threads);

	int sock = ::socket(AF_INET, SOCK_DGRAM, 0);

	sockaddr_in destination;
	destination.sin_family = AF_INET;
	destination.sin_port = htons(port);
	destination.sin_addr.s_addr = inet_addr(hostname.c_str());


    int n_cells = 100;
    area_matrix grid;
    grid.grid = grid._grid1;
    grid.future_grid = grid._grid2;
    generate_cells_uniform(grid, n_cells, {1, 40}, {1, 40}, 13);


    // main_loop(grid, sock, destination);
    for (int i = 0; i < n_updates; i++) {
        update(grid);
    }
    store_cells(grid, std::cout);

	::close(sock);

	return 0;
}
