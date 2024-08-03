#pragma once

#include <glm/vec2.hpp>
#include <tower/world/schema/tile_map_data.hpp>

#include <cmath>
#include <cstdint>
#include <format>
#include <fstream>
#include <vector>

namespace tower::world {
using Pixels = uint32_t;
using schema::TileType;
using schema::TileState;

struct Tile {
    static constexpr Pixels TILE_SIZE = 32;

    TileType type {TileType::NONE};
    TileState state {TileState::NONE};
};

class TileMap {
public:
    TileMap() = default;
    explicit TileMap(const glm::uvec2& size);

    TileMap(TileMap&& other) noexcept;
    TileMap& operator=(TileMap&& other) noexcept;

    static TileMap load_tile_map(std::string_view name);
    static glm::uvec2 position_to_indice(const glm::vec2& position);

    glm::uvec2 get_size() const { return _size; }

    Tile& get_tile(const size_t x, const size_t y) { return _tiles[y * _size.x + x]; }
    Tile& get_tile(const glm::uvec2& p) { return get_tile(p.x, p.y); }
    Tile& operator[](const size_t i) { return get_tile(i % _size.x, i / _size.x); }

    bool is_outside(const glm::vec2& p) const {
        return p.x < 0.0f || p.x > static_cast<float>(_size.x * Tile::TILE_SIZE)
            || p.y < 0.0f || p.y > static_cast<float>(_size.y * Tile::TILE_SIZE);
    }

    bool is_blocked(const glm::vec2& p) const {
        const auto indice = position_to_indice(p);
        return _tiles[indice.y * _size.x + indice.x].state == TileState::BLOCKED;
    }

private:
    glm::uvec2 _size {};
    std::vector<Tile> _tiles {};
};

inline TileMap::TileMap(const glm::uvec2& size)
    : _size {size}, _tiles {_size.x * _size.y} {}

inline TileMap::TileMap(TileMap&& other) noexcept
    : _size {other._size}, _tiles {std::move(other._tiles)} {}

inline TileMap& TileMap::operator=(TileMap&& other) noexcept {
    if (this != &other) {
        _size = other._size;
        _tiles = std::move(other._tiles);
    }
    return *this;
}

inline TileMap TileMap::load_tile_map(std::string_view name) {
    spdlog::info("[TileMap] Loading tile map {}", name);

    std::ifstream stream {std::format("{}/{}.bin", TILE_MAP_DATA_ROOT, name), std::ios::in | std::ios::binary};
    if (!stream.is_open()) {
        spdlog::error("[TileMap] Cannot open tile map {}", name);
        return {};
    }
    const std::vector<uint8_t> buffer {std::istreambuf_iterator<char> {stream}, std::istreambuf_iterator<char> {}};

    using namespace world::schema;
    const auto tile_map_data = GetTileMapData(buffer.data());
    if (flatbuffers::Verifier verifier {buffer.data(), buffer.size()}; !tile_map_data || !tile_map_data->
        Verify(verifier)) {
        spdlog::error("[TileMap] Invalid tile map {}", name);
        return {};
    }

    TileMap tile_map {glm::uvec2 {tile_map_data->size()->x(), tile_map_data->size()->y()}};
    const auto tiles = tile_map_data->tiles();
    if (const auto size = tile_map.get_size(); size.x * size.y != tiles->size()) {
        spdlog::error("[TileMap] Invalid tile map size {}", name);
        return {};
    }

    for (auto i {0}; i < tiles->size(); ++i) {
        const auto tile_data = tiles->Get(i);
        tile_map[i] = Tile {.type = tile_data->type(), .state = tile_data->state()};
    }

    spdlog::info("[TileMap] Loaded tile map {}", name);
    return tile_map;
}

inline glm::uvec2 TileMap::position_to_indice(const glm::vec2& position) {
    return glm::uvec2 {
        static_cast<uint32_t>(std::max(position.x, 0.0f)) / Tile::TILE_SIZE,
        static_cast<uint32_t>(std::max(position.y, 0.0f)) / Tile::TILE_SIZE
    };
}
}
