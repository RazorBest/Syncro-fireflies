#ifndef CELLS_H
#define CELLS_H

#include <cstdlib>
#include <vector>

#define MAX_VIEW 1000000
#define MAX_DIST 5

#define LIGHT_OFF 0
#define LIGHT_ON 1

#define T_MAX 900
#define T_MIN 30

typedef unsigned int uint;

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

    bool is_neighbour(const point_data &p) const; 

    void adapt_to_1(const point_data &p); 

    void adapt_to_2(const point_data &p); 

    void adapt_to_3(const point_data &p); 

    void adapt_to_4(const point_data &p); 

    void adapt_to(const point_data &p); 

    void adapt_to_given(const std::vector<point_data> v, int k,
            std::vector<const point_data*> far_neigh); 
};

struct area {
    std::vector<point_data> v;
};

struct area_line {
    area *line_start;
    size_t col;

    area& operator[](std::size_t idx) {
        return *(line_start + idx);
    }

    const area& operator[](std::size_t idx) const {
        return *(line_start + idx);
    }
};

struct const_area_line {
    const area *line_start;
    size_t col;

    const_area_line(const area *ptr, size_t col) {
        this->line_start = ptr;
        this->col = col;
    }

    const area& operator[](std::size_t idx) const {
        return *(line_start + idx);
    }
};

struct area_matrix;

struct area_matrix_view {
    const area *grid;
    size_t col;
    uint n;
    uint m;

    area_matrix_view(const area_matrix &source, uint offx, uint offy, uint n, uint m);
    area_matrix_view(const area *grid, uint col, uint offx, uint offy, uint n, uint m);
    const const_area_line operator[](std::size_t idx) const;
};

struct area_matrix {
    area *grid;
    size_t col;
    uint n;
    uint m;

    area_matrix(uint n, uint m) : n(n), m(m), col(m) {
        grid = new area[n * col];//(area*)malloc(n * col * sizeof(*grid));
    }

    area_matrix(uint col, uint n, uint m) : n(n), m(m), col(col) {
        grid = new area[n * col];//(area*)malloc(n * col * sizeof(*grid));
    }

    area_matrix(area_matrix &area) = delete;
    area_matrix(const area_matrix &area) = delete;
    area_matrix operator=(area_matrix &area) = delete;
    area_matrix operator=(const area_matrix &area) = delete;

    area_matrix(area_matrix &&area) {
        grid = area.grid;
        col = area.col;
        n = area.n;
        m = area.m;
        area.grid = nullptr;
    }
    area_matrix& operator=(area_matrix &&area) {
        if (this != &area) {
            col = area.col;
            n = area.n;
            m = area.m;
            delete[] grid;
            grid = area.grid;
            area.grid = nullptr;
        }

        return *this;
    }

    area_matrix_view submatrix(uint offx, uint offy, uint n, uint m) const {
        return area_matrix_view(*this, offx, offy, n, m);
    }

    area_line operator[](std::size_t idx) {
        area_line line;
        line.col = col;
        line.line_start = grid + idx * col;
        return line;
    }

    const const_area_line operator[](std::size_t idx) const {
        const area *ptr = grid + idx * col;
        const_area_line line(ptr, col);
        return line;
    }

    ~area_matrix() {
        delete[] grid;
    }
};

struct simulation_area {
    area_matrix grid;
    area_matrix future_grid;
    // Each area will cover a 10x10 square
    // For this algorithm, it should be  between
    // 2/3 * l and l*sqrt(2)
    // Where l is MAX_DIST
    float area_length = 10;

    simulation_area(uint n, uint m) : grid(n, m), future_grid(n, m) {}
    void swap_state() {
        std::swap(grid, future_grid);
    }
};

struct sendable_point_data {
	uint id;
	float x, y;
	int t1, t2;
    int light;
    int cycle;
	int counter;
};

// This is a struct used just for viewing
// It must not be instantiated
struct sendable_area {
    int n;
    sendable_point_data v[MAX_VIEW];

    sendable_area() = delete;
};

// Serialize point_data
void point_data_to_sendable(char *sendable, const point_data &cell);
point_data sendable_to_point_data(const sendable_point_data &sendable);

// Serialize area
void area_convert_to_sendable(char *buf, int *size, const area& a); 
char* sendable_convert_to_area(char *buf, area &a);

// Serialize area_matrix and area_matrix_view
void area_matrix_view_convert_to_sendable(char *buf, int *size, const area_matrix_view& grid); 
area_matrix sendable_convert_to_area_matrix(char *buf, int n, int m); 
void sendable_convert_to_area_submatrix(char *buf, area_matrix &grid,
      uint offx, uint offy, int n, int m);

void generate_cells_uniform(simulation_area &grid, int n,
        const std::pair<float, float> &x_limits,
        const std::pair<float, float> &y_limits,
        unsigned int seed);

void update_cell(area_matrix &grid, int i, int j, int k,
        point_data &cell);
void update_simulation(simulation_area &game, int offx, int offy, int n, int m);
void update_simulation_and_swap(simulation_area &game, int offx, int offy, int n, int m);

void store_cells(const area_matrix &grid, std::ostream &out);

#endif // CELLS_H
