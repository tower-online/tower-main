#include <spire/game/entity.hpp>

namespace spire::game {
Entity::Entity()
    : id(generate_id()) {}

uint32_t Entity::generate_id() {
    static std::atomic<uint32_t> id_generator {0};

    return ++id_generator;
}
}
