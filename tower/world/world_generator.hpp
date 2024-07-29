#pragma once

#include <tower/world/tile_map.hpp>

#include <string_view>

namespace tower::world {
class WorldGenerator {
public:
    static std::optional<TileMap> load_tile_map(std::string_view name);
};
}