#pragma once

#include <cmath>
#include <functional>
#include <vector>

namespace tower {
struct Point {
    int x, y;

    struct Hash {
        int operator()(const Point& p) const {
            return std::hash<int> {}(p.x) ^ (std::hash<int> {}(p.y) << 1);
        }
    };

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }

    Point operator+(const Point& other) const {
        return Point {x + other.x, y + other.y};
    }

    static int manhattan_distance(const Point p1, const Point p2) {
        return std::abs(p1.x - p2.x) + std::abs(p1.y - p2.y);
    }

    static float euclidean_distance(const Point p1, const Point p2) {
        return sqrtf(
            static_cast<float>((p1.x - p2.x) * (p1.x - p2.x)) + static_cast<float>((p1.y - p2.y) * (p1.y - p2.y)));
    }
};

template <typename T>
class Grid {
public:
    Grid(const size_t size_x, const size_t size_y,
        std::function<bool(const Point& p, const T& x)>&& is_blocked_internal = [] { return false; })
        : data {}, size_x {size_x}, size_y {size_y}, is_blocked_internal {std::move(is_blocked_internal)} {
        data.resize(size_x * size_y);
    }

    Grid(std::vector<T>&& data, const size_t size_x, const size_t size_y,
        std::function<bool(const Point& p, const T& x)>&& is_blocked_internal = [] { return false; })
        : data {std::move(data)}, size_x {size_x}, size_y {size_y},
        is_blocked_internal {std::move(is_blocked_internal)} {}

    const T& at(const Point& p) const { return data[p.y * size_x + p.x]; }
    T& at(const Point& p) { return data[p.y * size_x + p.x]; }
    T& at(const size_t i) { return data[i]; }

    bool is_inside(const Point& p) const {
        return p.x >= 0 && p.x < size_x && p.y >= 0 && p.y < size_y;
    }

    bool is_blocked(const Point& p) const {
        return is_blocked_internal(p, at(p));
    }

    std::vector<Point> get_neighbors(const Point& p) const {
        const auto x = p.x, y = p.y;
        std::vector<Point> neighbors {};

        if (x < size_x - 1) neighbors.emplace_back(x + 1, y);
        if (x >= 1) neighbors.emplace_back(x - 1, y);
        if (y < size_y - 1) neighbors.emplace_back(x, y + 1);
        if (y >= 1) neighbors.emplace_back(x, y - 1);
        if (x < size_x - 1 && y < size_y - 1) neighbors.emplace_back(x + 1, y + 1);
        if (x < size_x - 1 && y >= 1) neighbors.emplace_back(x + 1, y - 1);
        if (x >= 1 && y < size_y - 1) neighbors.emplace_back(x - 1, y + 1);
        if (x >= 1 && y >= 1) neighbors.emplace_back(x - 1, y - 1);

        return neighbors;
    }

    const size_t size_x, size_y;

private:
    std::vector<T> data;
    const std::function<bool(const Point& p, const T& x)> is_blocked_internal;
};
}
