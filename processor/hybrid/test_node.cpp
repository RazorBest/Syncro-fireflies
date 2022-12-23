#include <iostream>

struct node_info {
    int x, y;
    int n, m;
};

node_info get_intersection(node_info &src, node_info &dst) {
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

int main() {

    node_info src;
    node_info dst;

    src = {5, 6, 3, 5};
    dst = {0, 1, 6, 10};
    node_info inter = get_intersection(src, dst);
    std::cout << inter.x << ' ' << inter.y << ' ' << inter.n << ' ' << inter.m << '\n';

    return 0;
}
