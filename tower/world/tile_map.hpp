#pragma once

#include <glm/vec2.hpp>
#include <spdlog/spdlog.h>
#include <tower/system/container/grid.hpp>
#include <tower/world/schema/tile_map_data.hpp>

#include <format>
#include <fstream>

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
    TileMap();
    explicit TileMap(const glm::uvec2& size);

    // TileMap(TileMap&& other) noexcept;
    // TileMap& operator=(TileMap&& other) noexcept;

    static TileMap load_tile_map(std::string_view name);
    static Point position_to_point(const glm::vec2& position);

    glm::uvec2 get_size() const { return {_grid.size_x, _grid.size_y}; }
    const Grid<Tile>& get_grid() const { return _grid; }
    std::string_view get_name() const { return _name; }

    Tile& at(const Point& p) { return _grid.at(p); }
    Tile& at(const glm::vec2& p) { return at(position_to_point(p)); }
    Tile& at(const size_t i) { return _grid.data[i]; }
    const Tile& at(const Point& p) const { return _grid.at(p); }
    const Tile& at(const glm::vec2& p) const { return at(position_to_point(p)); }

    bool try_at(const glm::vec2& p, Tile& tile);

private:
    Grid<Tile> _grid;
    std::string _name {};
};

inline TileMap::TileMap()
    : _grid {0, 0, [this](const Point& p) { return at(p).state == TileState::BLOCKED; }} {}

inline TileMap::TileMap(const glm::uvec2& size)
    : _grid {size.x, size.y, [this](const Point& p) { return at(p).state == TileState::BLOCKED; }} {}

// inline TileMap::TileMap(TileMap&& other) noexcept
//     : _size {other._size}, _tiles {std::move(other._tiles)} {}
//
// inline TileMap& TileMap::operator=(TileMap&& other) noexcept {
//     if (this != &other) {
//         _size = other._size;
//         _tiles = std::move(other._tiles);
//     }
//     return *this;
// }

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
    tile_map._name = name;

    const auto tiles = tile_map_data->tiles();
    if (const auto size = tile_map.get_size(); size.x * size.y != tiles->size()) {
        spdlog::error("[TileMap] Invalid tile map size {}", name);
        return {};
    }

    for (auto i {0}; i < tiles->size(); ++i) {
        const auto tile_data = tiles->Get(i);
        tile_map.at(i) = Tile {.type = tile_data->type(), .state = tile_data->state()};
    }

    spdlog::info("[TileMap] Loaded tile map {}", name);
    return tile_map;
}

inline Point TileMap::position_to_point(const glm::vec2& position) {
    return Point {
        static_cast<int>(std::max(position.x, 0.0f)) / static_cast<int>(Tile::TILE_SIZE),
        static_cast<int>(std::max(position.y, 0.0f)) / static_cast<int>(Tile::TILE_SIZE)
    };
}

inline bool TileMap::try_at(const glm::vec2& p, Tile& tile) {
    const auto point = position_to_point(p);

    if (!_grid.is_inside(point)) return false;

    tile = at(point);
    return true;
}
}
