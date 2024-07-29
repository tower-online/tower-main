#pragma once

#include <glm/vec2.hpp>
#include <tower/world/schema/types.hpp>

#include <cmath>
#include <cstdint>
#include <vector>

namespace tower::world {
using Pixels = uint32_t;

static constexpr Pixels TILE_SIZE = 32;

struct Tile {
    TileType type {TileType::NONE};
    TileState state {TileState::NONE};
};

class TileMap {
public:
    TileMap() = default;
    explicit TileMap(const glm::uvec2& size)
        : _size {size}, _tiles {_size.x * _size.y} {}

    TileMap(TileMap&& other) noexcept
        : _size {other._size}, _tiles {std::move(other._tiles)} {}

    TileMap& operator=(TileMap&& other) noexcept {
        if (this != &other) {
            _size = other._size;
            _tiles = std::move(other._tiles);
        }
        return *this;
    }

    static glm::uvec2 position_to_indice(const glm::vec2& position) {
        return glm::uvec2 {
            static_cast<uint32_t>(std::max(position.x, 0.0f)) / TILE_SIZE,
            static_cast<uint32_t>(std::max(position.y, 0.0f)) / TILE_SIZE
        };
    }

    glm::uvec2 get_size() const { return _size; }

    Tile& get_tile(const size_t x, const size_t y) { return _tiles[y * _size.x + x]; }
    Tile& get_tile(const glm::uvec2& p) { return get_tile(p.x, p.y); }
    Tile& operator[](const size_t i) { return get_tile(i % _size.x, i / _size.x); }



private:
    glm::uvec2 _size {};
    std::vector<Tile> _tiles {};
};
}
