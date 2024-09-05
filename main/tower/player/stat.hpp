#pragma once

#include <boost/asio.hpp>

namespace tower::player {
class Stat {
public:
    enum class Type : uint8_t {
        STR, MAG, AGI, CON,
        FAITH
    };

    std::optional<int> get(Type type) const;

    void add(Type type, int value);
    boost::asio::awaitable<void> add_update(Type type, int value);

    void set(Type type, int value);
    boost::asio::awaitable<void> set_update(Type type, int value);

private:
    std::unordered_map<Type, int> _stats {
        {Type::STR, 0},
        {Type::MAG, 0},
        {Type::AGI, 0},
        {Type::CON, 0}
    };
};
}
