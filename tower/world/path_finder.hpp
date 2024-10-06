#pragma once

#include <tower/system/container/grid.hpp>

#include <algorithm>
#include <functional>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace tower::world {
class Pathfinder {
public:
    enum class Algorithm { ASTAR, JPS };

    template <typename T>
    static std::vector<Point> find_path(const Grid<T>& grid, Point start, Point end,
        Algorithm algorithm = Algorithm::ASTAR);

private:
    template <typename T>
    static std::vector<Point> astar(const Grid<T>& grid, Point start, Point end);

    template <typename T>
    static std::vector<Point> jps(const Grid<T>& grid, Point start, Point end);

    static std::vector<Point> reconstruct_path(const std::unordered_map<Point, Point, Point::Hash>& parents, Point end);
};

template <typename T>
std::vector<Point> Pathfinder::find_path(const Grid<T>& grid, const Point start, const Point end,
    const Algorithm algorithm) {
    if (algorithm == Algorithm::ASTAR) {
        return astar(grid, start, end);
    }
    if (algorithm == Algorithm::JPS) {
        return jps(grid, start, end);
    }
    return {};
}

template <typename T>
std::vector<Point> Pathfinder::astar(const Grid<T>& grid, const Point start, const Point end) {
    struct Node {
        int f;
        Point p;
    };

    if (!grid.is_inside(start) || !grid.is_inside(end)) return {};

    const auto node_cmp = [](const Node& n1, const Node& n2) { return n1.f > n2.f; };
    const auto heuristic = [](const Point p1, const Point p2) { return Point::manhattan_distance(p1, p2); };

    std::priority_queue<Node, std::vector<Node>, decltype(node_cmp)> open {node_cmp};
    std::unordered_set<Point, Point::Hash> closed {};
    std::unordered_map<Point, Point, Point::Hash> parents {};
    std::unordered_map<Point, int, Point::Hash> g {{start, 0}};

    open.push({0, start});
    while (!open.empty()) {
        const auto [_, current] = open.top();
        open.pop();
        closed.insert(current);

        if (current == end) return reconstruct_path(parents, end);

        for (const auto next : grid.get_neighbors(current)) {
            if (closed.contains(next)) continue;
            if (grid.is_blocked(next)) continue;

            const auto g_next = g[current] + heuristic(current, next);
            if (g.contains(next) && g_next >= g[next]) continue;

            parents[next] = current;

            g[next] = g_next;
            const auto f = g_next + heuristic(next, end);
            open.push({f, next});
        }
    }

    return {};
}

template <typename T>
std::vector<Point> Pathfinder::jps(const Grid<T>& grid, const Point start, const Point end) {
    struct Node {
        int f;
        Point p;
    };
    const auto node_cmp = [](const Node& n1, const Node& n2) { return n1.f > n2.f; };
    const auto heuristic = [](const Point p1, const Point p2) { return Point::manhattan_distance(p1, p2); };

    std::priority_queue<Node, std::vector<Node>, decltype(node_cmp)> open {node_cmp};
    std::unordered_set<Point, Point::Hash> closed {};
    std::unordered_map<Point, Point, Point::Hash> parents {};

    const auto has_forced_neighbor = [&](const Point block, const Point direction)->bool {
        const auto forced_neighbor = block + direction;

        if (!grid.is_inside(block) || !grid.is_blocked(block)) return false;
        if (!grid.is_inside(forced_neighbor) || grid.is_blocked(forced_neighbor)) return false;
        return true;
    };

    const auto probe = [&](Point p, const Point direction, auto&& probe_r)->bool {
        const auto [dx, dy] = direction;

        while (grid.is_inside(p) && !grid.is_blocked(p)) {
            parents[p] = {p.x - dx, p.y - dy};
            if (p == end) {
                // open.push({0, end});
                return true;
            }

            if (dx * dy != 0) {
                // Diagonal probe
                const Point right {p.x, p.y - dy};
                const Point left {p.x - dx, p.y};

                if (has_forced_neighbor(right, {dx, 0}) || has_forced_neighbor(left, {0, dy})) {
                    open.push({heuristic(p, end), p});
                    return true;
                }

                bool found_forced_neighbor = false;
                found_forced_neighbor |= probe_r({p.x + dx, p.y}, {dx, 0}, probe_r);
                found_forced_neighbor |= probe_r({p.x, p.y + dy}, {0, dy}, probe_r);

                if (found_forced_neighbor) {
                    open.push({heuristic(p, end), p});
                    return true;
                }
            } else if (dx != 0) {
                // Horizontal probe
                const Point up {p.x, p.y + 1};
                const Point down {p.x, p.y - 1};

                if (has_forced_neighbor(up, direction) || has_forced_neighbor(down, direction)) {
                    open.push({heuristic(p, end), p});
                    return true;
                }
            } else if (dy != 0) {
                // Vertical probe
                const Point right {p.x + 1, p.y};
                const Point left {p.x - 1, p.y};

                if (has_forced_neighbor(right, direction) || has_forced_neighbor(left, direction)) {
                    open.push({heuristic(p, end), p});
                    return true;
                }
            }

            closed.insert(p);
            p.x += dx;
            p.y += dy;
        }
        return false;
    };

    const auto jump = [&grid, &closed, &probe](const Point p) {
        closed.insert(p);
        for (const auto neighbor : grid.get_neighbors(p)) {
            if (closed.contains(neighbor) || grid.is_blocked(neighbor)) continue;
            probe(neighbor, {neighbor.x - p.x, neighbor.y - p.y}, probe);
        }
    };

    open.push({0, start});
    while (!open.empty()) {
        const auto [_f, current] = open.top();
        open.pop();
        jump(current);

        if (parents.contains(end)) return reconstruct_path(parents, end);
    }

    return {};
}

inline std::vector<Point> Pathfinder::reconstruct_path(
    const std::unordered_map<Point, Point, Point::Hash>& parents, const Point end) {
    std::vector<Point> path {end};
    auto current = end;
    while (parents.contains(current)) {
        current = parents.at(current);
        path.push_back(current);
    }
    std::reverse(std::begin(path), std::end(path));
    return path;
}
}
