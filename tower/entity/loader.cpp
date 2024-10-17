#include <spdlog/spdlog.h>
#include <tower/entity/loader.hpp>

#include <tower/entity//simple_monster.hpp>

namespace tower::entity {

void Loader::load_all() {
    spdlog::info("[entity::Loader] Loading...");

    SimpleMonster::Data::load();

    spdlog::info("[entity::Loader] Loading done");
}
}
