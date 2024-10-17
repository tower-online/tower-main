#include <boost/json.hpp>
#include <tower/item/equipment/fist.hpp>
#include <tower/physics/cube_collision_shape.hpp>

#include <fstream>
#include <sstream>

namespace tower::item {
Fist::Fist()
    : Equipment {ItemType::FIST} {}

std::unique_ptr<Fist> Fist::create() {
    auto fist {std::make_unique<Fist>()};
    fist->node = std::make_shared<world::WorldObject>();

    // TODO: Factory to set damage, rarity, and so on.
    fist->_melee_attack_damage = 1;
    fist->_melee_attack_shape = std::make_shared<physics::CubeCollisionShape>(Data::attack_shape_size());
    fist->node->add_child(fist->_melee_attack_shape);

    return fist;
}

void Fist::Data::load() {
    std::ifstream f {file.data()};
    std::stringstream buffer;
    buffer << f.rdbuf();

    boost::json::value parsed {boost::json::parse(buffer.view())};
    boost::json::object& obj {parsed.as_object()};

    _attack_interval = milliseconds {obj["attackInterval"].as_int64()};
    {
        auto& size {obj["attackShape"].as_array()};
        _attack_shape_size.x = static_cast<float>(size.at(0).as_double());
        _attack_shape_size.y = static_cast<float>(size.at(1).as_double());
        _attack_shape_size.z = static_cast<float>(size.at(2).as_double());
    }
}
}
