#include <boost/json.hpp>
#include <tower/item/equipment/weapon/fist.hpp>
#include <tower/physics/cube_collision_shape.hpp>

#include <fstream>
#include <sstream>

namespace tower::item::equipment {
std::shared_ptr<Fist> Fist::create() {
    auto fist {std::make_shared<Fist>()};
    fist->node = std::make_shared<world::Node>();

    // TODO: Factory to set damage, rarity, and so on.
    fist->_damage = 1;
    fist->_attack_shape = std::make_shared<physics::CubeCollisionShape>(Data::attack_shape_size());
    fist->node->add_child(fist->_attack_shape);

    return fist;
}

void Fist::Data::load() {
    std::ifstream f {file.data()};
    std::stringstream buffer;
    buffer << f.rdbuf();

    boost::json::value parsed {boost::json::parse(buffer.view())};
    boost::json::object& obj {parsed.as_object()};

    _attack_interval = milliseconds {obj["attackInterval"].as_int64()};
}
}
