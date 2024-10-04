#pragma once

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

    T& at(const int r, const int c) { return _data[r * cols + c]; }
    T& at(const int i) { return _data[i]; }

    bool is_inside(const int r, const int c) const {
        return r >= 0 && c >= 0 && r < rows && c < cols;
    }

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
