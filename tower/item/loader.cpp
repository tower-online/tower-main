#include <spdlog/spdlog.h>
#include <tower/item/loader.hpp>

#include <tower/item/equipment/fist.hpp>

namespace tower::item {

void Loader::load_all() {
    spdlog::info("[item::Loader] Loading...");

    Fist::Data::load();

    spdlog::info("[item::Loader] Loading done");
}
}
