#include <iostream>
#include <cmath>

#include "./cells.h"
// such that lo <= x <= hi
int rand_int(int lo, int hi) {
    return lo + (rand() % (hi - lo + 1));
}

// Returns a random floating point number x
// such that lo <= x <= hi
float rand_float(float lo, float hi) {
    return lo + (rand()) * (hi - lo) / (float)RAND_MAX;
}

bool point_data::is_neighbour(const point_data &p) const {
    return (x-p.x)*(x-p.x) + (y-p.y)*(y-p.y) <= MAX_DIST*MAX_DIST;
}

void point_data::adapt_to_1(const point_data &p) {
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

void point_data::adapt_to_2(const point_data &p) {
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

void point_data::adapt_to_3(const point_data &p) {
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

void point_data::adapt_to_4(const point_data &p) {
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

void point_data::adapt_to(const point_data &p) {
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

void point_data::adapt_to_given(const std::vector<point_data> v, int k,
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

std::vector<const point_data*> get_far_neighbours(area_matrix &grid, int i, int j,
        int k, const point_data &cell) {
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
        if (i + di < 0 || i + di >= grid.n) {
            continue;
        }
        if (j + dj < 0 || j + dj >= grid.m) {
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

void update_cell(area_matrix &grid, int i, int j, int k,
        point_data &cell) {
    auto light = cell.light;
    auto cycle = cell.cycle;
    int index = -1; 

    /*
    int neigh_count = count_neighbours(g, i, j, k, cell);

    if (cell.cycle >= neigh_count) {
        cell.cycle = 0;
    }

    if (neigh_count > 0) {
        const point_data &neigh = get_nth_neighbour(g, i, j, k, cell, cell.cycle);
        cell.adapt_to(neigh);
    }
    */

    auto far_neighbours = get_far_neighbours(grid, i, j, k, cell);

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

void update_simulation(simulation_area &game, int offx, int offy, int n, int m) {
    for (int i = offx; i < offx + n; i++) {
        for (int j = offy; j < offy + m; j++) {
            for (int k = 0; k < game.grid[i][j].v.size(); k++) {
                game.future_grid[i][j].v[k] = game.grid[i][j].v[k];
                update_cell(game.grid, i, j, k, game.future_grid[i][j].v[k]);
            }
        }
    }
}

void update_simulation_and_swap(simulation_area &game, int offx, int offy, int n, int m) {
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

void point_data_to_sendable(char *buf, const point_data &cell) {
    sendable_point_data *sendable = (sendable_point_data*)buf;

    sendable->id = cell.id;
    sendable->t1 = cell.t1;
    sendable->t2 = cell.t2;
    sendable->x = cell.x;
    sendable->y = cell.y;
    sendable->light = cell.light;
    sendable->counter = cell.counter;
    sendable->cycle = cell.cycle;
}

point_data sendable_to_point_data(const sendable_point_data &sendable) {
    point_data cell;

    cell.id = sendable.id;
    cell.t1 = sendable.t1;
    cell.t2 = sendable.t2;
    cell.x = sendable.x;
    cell.y = sendable.y;
    cell.light = sendable.light;
    cell.counter = sendable.counter;
    cell.cycle = sendable.cycle;

    return cell;
}


void area_convert_to_sendable(char *buf, int *size, const area& a) {
    sendable_area *sendable = (sendable_area*)buf;

    int n = 0;
    for (const auto &cell : a.v) {
        point_data_to_sendable((char*)&sendable->v[n], cell);
        n++;
    }

    sendable->n = n;
    *size = (((char*)&sendable->v[n]) - (char*)sendable);
}

char* sendable_convert_to_area(char *buf, area &a) {
    sendable_area *sendable = (sendable_area*)buf;

    a = area();

    for (int i = 0; i < sendable->n; i++) {
        a.v.push_back(sendable_to_point_data(sendable->v[i]));
    }

    return (char*) &sendable->v[sendable->n];
}

area_matrix_view::area_matrix_view(const area_matrix &source, uint offx, uint offy, uint n, uint m) : n(n), m(m) {
    col = source.col;
    grid = source.grid + offx * col + offy;
}

area_matrix_view::area_matrix_view(const area *grid, uint col, uint offx, uint offy, uint n, uint m) : n(n), m(m), col(col) {
    col = col;
    this->grid = grid + offx * col + offy;
}

const const_area_line area_matrix_view::operator[](std::size_t idx) const {
    const area *ptr = grid + idx * col;
    const_area_line line(ptr, col);

    return line;
}

void area_matrix_view_convert_to_sendable(char *buf, int *size, const area_matrix_view& grid) {
    *size = 0;

    for (int i = 0; i < grid.n; i++) {
        for (int j = 0; j < grid.m; j++) {
            int area_size;
            area_convert_to_sendable(buf + *size, &area_size, grid[i][j]);

            *size += area_size;
        }
    }
}

area_matrix sendable_convert_to_area_matrix(char *buf, int n, int m) {
    area_matrix grid(n, m);
    char *ptr = buf;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            ptr = sendable_convert_to_area(ptr, grid[i][j]);
        }
    }

    return grid;
}

void sendable_convert_to_area_submatrix(char *buf, area_matrix &grid,
      uint offx, uint offy, int n, int m) {
    char *ptr = buf;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            ptr = sendable_convert_to_area(ptr, grid[offx + i][offy + j]);
        }
    }
}

// Generate n cells
void generate_cells(simulation_area &game, int n, 
        const std::pair<float, float> &x_limits,
        const std::pair<float, float> &y_limits,
        unsigned int seed) {
    srand(seed);

    if (x_limits.first > x_limits.second || 
            y_limits.first > y_limits.second) {
        std::cout << "Generate cells: error in limits\n";
        return;
    }

    for (uint i = 0; i < n; i++) {
        float x = rand_float(x_limits.first, x_limits.second);
        float y = rand_float(y_limits.first, y_limits.second);
        int t1 = rand_int(T_MIN, T_MAX);
        int t2 = rand_int(T_MIN, T_MAX);

        int area_i = floor(x / game.area_length);
        int area_j = floor(y / game.area_length);

        if (area_i < 0 || area_j < 0 || area_i >= game.grid.n || area_j >= game.grid.m) {
            std::cout << "Generate cells: area out of bounds\n";
            continue;
        }

        point_data cell{i, t1, t2, x, y, LIGHT_ON, 0};
        game.grid[area_i][area_j].v.push_back(cell);
        game.future_grid[area_i][area_j].v.push_back(cell);
    }
}

// Generate n cells
void generate_cells_uniform(simulation_area &grid, int n, 
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

    uint index = 0;
    
    while (n) {
        for (float x = x_limits.first; x < x_limits.second && n; x += sq_l) {
            for (float y = y_limits.first; y < y_limits.second && n; y += sq_l) {
                int t1 = rand_int(T_MIN, T_MAX);
                int t2 = rand_int(T_MIN, T_MAX);

                int area_i = floor(x / grid.area_length);
                int area_j = floor(y / grid.area_length);

                if (area_i < 0 || area_j < 0 || area_i >= grid.grid.n || area_j >= grid.grid.m) {
                    std::cout << "Generate cells: area out of bounds\n";
                    continue;
                }

                point_data cell{index, t1, t2, x, y, LIGHT_ON, 0};
                grid.grid[area_i][area_j].v.push_back(cell);
                grid.future_grid[area_i][area_j].v.push_back(cell);

                n--;
                index++;
            }
        }
    }
}

void store_cells(const area_matrix &grid, std::ostream &out) {
    int count = 0;
    for (int i = 0; i < grid.n; i++) {
        for (int j = 0; j < grid.m; j++) {
            for (const auto &cell : grid[i][j].v) {
                count++;
            }
        }
    }
    out << count << '\n';

    for (int i = 0; i < grid.n; i++) {
        for (int j = 0; j < grid.m; j++) {
            for (const auto &cell : grid[i][j].v) {
                    out << cell.t1 << ' ' << cell.t2 << ' '; 
                    out << cell.counter << ' '; 
                    out << cell.x << ' ' << cell.y << ' '; 
                    out << unsigned(cell.light) << '\n';
                }
        }
    }
}
