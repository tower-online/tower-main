#include <flatbuffers/flatbuffers.h>
#include <spdlog/spdlog.h>
#include <tower/world/world_generator.hpp>
#include <tower/world/schema/tile_map_data.hpp>

#include <format>
#include <fstream>

namespace tower::world {
std::optional<TileMap> WorldGenerator::load_tile_map(std::string_view name) {
    std::ifstream stream {std::format("{}/{}", TILE_MAP_DATA_ROOT, name), std::ios::in | std::ios::binary};
    if (!stream.is_open()) {
        spdlog::error("[WorldGenerator] Cannot open tile map {}", name);
        return {};
    }
    const std::vector<uint8_t> buffer {std::istreambuf_iterator<char> {stream}, std::istreambuf_iterator<char> {}};

    using namespace world::schema;
    const auto tile_map_data = GetTileMapData(buffer.data());
    if (flatbuffers::Verifier verifier {buffer.data(), buffer.size()}; !tile_map_data || tile_map_data->Verify(verifier)) {
        spdlog::error("[WorldGenerator] Invalid tile map {}", name);
        return {};
    }

    TileMap tile_map {glm::uvec2 {tile_map_data->size()->x() , tile_map_data->size()->y()}};
    const auto tiles = tile_map_data->tiles();
    if (const auto size = tile_map.get_size(); size.x * size.y != tiles->size()) {
        spdlog::error("[WorldGenerator] Invalid tile map size {}", name);
        return {};
    }

    for (auto i {0}; i < tiles->size(); ++i) {
        const auto tile_data = tiles->Get(i);
        tile_map[i] = Tile {.type = tile_data->type(), .state = tile_data->state()};
    }

    return tile_map;
}
}
