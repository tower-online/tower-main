#pragma once

#include <cmath>
#include <vector>

namespace tower {
template <typename T>
class Grid {
public:
    Grid(const size_t rows, const size_t cols)
        : rows {rows}, cols {cols}, _data {} {
        _data.resize(rows * cols);
    }

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
    typename std::vector<T>::reference at(const size_t i) { return _data[i]; }

    typename std::vector<T>::const_reference at(const size_t r, const size_t c) const { return _data.at(r * cols + c); }
    typename std::vector<T>::const_reference at(const size_t i) const { return _data.at(i); }

    bool is_inside(const int r, const int c) const {
        return r >= 0 && c >= 0 && r < static_cast<int>(rows) && c < static_cast<int>(cols);
    }

    bool is_inside(const size_t i) const { return i < rows * cols; }

    std::vector<std::pair<int, int>> get_4way_adjacents(const int r, const int c) const {
        if (!is_inside(r, c)) return {};
        std::vector<std::pair<int, int>> adjacents {};

        if (r < rows - 1) adjacents.emplace_back(r + 1, c);
        if (r >= 1) adjacents.emplace_back(r - 1, c);
        if (c < cols - 1) adjacents.emplace_back(r, c + 1);
        if (c >= 1) adjacents.emplace_back(r, c - 1);

        return adjacents;
    }

    std::vector<std::pair<int, int>> get_8way_adjacents(const int r, const int c) const {
        if (!is_inside(r, c)) return {};
        std::vector<std::pair<int, int>> adjacents {};

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

    const size_t rows, cols;

private:
    std::vector<T> _data;
};
}
