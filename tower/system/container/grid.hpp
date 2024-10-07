#pragma once

#include <boost/functional/hash.hpp>

#include <cmath>
#include <vector>

namespace tower {
struct Point {
    int r, c;

    struct Hash {
        size_t operator() (const Point& p) const {
            size_t seed {0};
            boost::hash_combine(seed, p.r);
            boost::hash_combine(seed, p.c);
            return seed;
        }
    };

    Point() = default;

    Point(const size_t r, const size_t c)
        : r {static_cast<int>(r)}, c {static_cast<int>(c)} {}

    Point(const int r, const int c)
        : r {r}, c {c} {}

    Point(const float fr, const float fc)
        : r {static_cast<int>(std::floor(fr))}, c {static_cast<int>(std::floor(fc))} {}

    bool operator==(const Point& other) const {
        return r == other.r && c == other.c;
    }

    Point operator+(const Point& other) const {
        return Point {r + other.r, c + other.c};
    }

    static int manhattan_distance(const Point p1, const Point p2) {
        return std::abs(p1.r - p2.r) + std::abs(p1.c - p2.c);
    }

    static float euclidean_distance(const Point p1, const Point p2) {
        return sqrtf(
            static_cast<float>((p1.r - p2.r) * (p1.r - p2.r)) + static_cast<float>((p1.c - p2.c) * (p1.c - p2.c)));
    }
};

template <typename T>
class Grid {
public:
    Grid(const size_t rows, const size_t cols)
        : rows {rows}, cols {cols}, _data {} {
        _data.resize(rows * cols);
    }

    Grid(std::vector<T>&& data, const size_t rows, const size_t cols)
        : rows {rows}, cols {cols}, _data {std::move(data)} {}

    Grid(Grid&& other) noexcept
        : rows {other.rows}, cols {other.cols}, _data {std::move(other._data)} {}

    Grid& operator=(Grid&& other) noexcept {
        if (this != &other) {
            _data = std::move(other._data);
            rows = other.rows;
            cols = other.cols;
        }
        return *this;
    }

    typename std::vector<T>::reference at(const size_t r, const size_t c) { return _data[r * cols + c]; }
    typename std::vector<T>::reference at(const Point& p) { return at(p.r, p.c); }
    typename std::vector<T>::reference at(const size_t i) { return _data[i]; }

    typename std::vector<T>::const_reference at(const size_t r, const size_t c) const { return _data.at(r * cols + c); }
    typename std::vector<T>::const_reference at(const Point& p) const { return at(p.r, p.c); }
    typename std::vector<T>::const_reference at(const size_t i) const { return _data.at(i); }

    [[nodiscard]] bool is_inside(const int r, const int c) const {
        return r >= 0 && c >= 0 && r < static_cast<int>(rows) && c < static_cast<int>(cols);
    }
    [[nodiscard]] bool is_inside(const Point& p) const { return is_inside(p.r, p.c); }
    [[nodiscard]] bool is_inside(const size_t i) const { return i < rows * cols; }

    [[nodiscard]] bool is_blocked(const int r, const int c) const { return static_cast<bool>(at(r, c)); }
    [[nodiscard]] bool is_blocked(const Point& p) const { return static_cast<bool>(at(p)); }
    [[nodiscard]] bool is_blocked(const size_t i) const { return static_cast<bool>(at(i)); }

    [[nodiscard]] std::vector<Point> get_4way_adjacents(const size_t r, const size_t c) const {
        if (!is_inside(r, c)) return {};
        std::vector<Point> adjacents {};

        if (r < rows - 1) adjacents.emplace_back(r + 1, c);
        if (r >= 1) adjacents.emplace_back(r - 1, c);
        if (c < cols - 1) adjacents.emplace_back(r, c + 1);
        if (c >= 1) adjacents.emplace_back(r, c - 1);

        return adjacents;
    }

    [[nodiscard]] std::vector<Point> get_4way_adjacents(const Point& p) const {
        return get_4way_adjacents(p.r, p.c);
    }

    [[nodiscard]] std::vector<Point> get_8way_adjacents(const size_t r, const size_t c) const {
        if (!is_inside(r, c)) return {};
        std::vector<Point> adjacents {};

        if (r < rows - 1) adjacents.emplace_back(r + 1, c);
        if (r >= 1) adjacents.emplace_back(r - 1, c);
        if (c < cols - 1) adjacents.emplace_back(r, c + 1);
        if (c >= 1) adjacents.emplace_back(r, c - 1);

        if (r < rows - 1 && c < cols - 1) adjacents.emplace_back(r + 1, c + 1);
        if (r < rows - 1 && c >= 1) adjacents.emplace_back(r + 1, c - 1);
        if (r >= 1 && c < cols - 1) adjacents.emplace_back(r - 1, c + 1);
        if (r >= 1 && c >= 1) adjacents.emplace_back(r - 1, c - 1);

        return adjacents;
    }

    [[nodiscard]] std::vector<Point> get_8way_adjacents(const Point& p) const {
        return get_8way_adjacents(p.r, p.c);
    }

    const size_t rows, cols;

private:
    std::vector<T> _data;
};
}
