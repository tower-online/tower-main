#pragma once

#include <boost/mysql.hpp>
#include <tower/network/packet/player_types.hpp>

namespace tower::player {
using StatType = network::packet::PlayerStatType;

static std::string_view type_to_string(StatType type);

template <std::integral T>
class Stat {
public:
    explicit Stat(const StatType type)
        : _type {type} {}

    T get() const { return _value; }
    void set(const T value) { _value = value; }
    boost::asio::awaitable<void> set_and_update(boost::mysql::pooled_connection& conn, T value, uint32_t character_id);
    boost::asio::awaitable<void> update(boost::mysql::pooled_connection& conn, uint32_t character_id);

    StatType type() const { return _type; }

private:
    StatType _type;
    T _value {};
};

struct Stats {
    Stat<int16_t> level {StatType::LEVEL};
    Stat<int32_t> exp {StatType::EXP};

    Stat<int16_t> str {StatType::STR};
    Stat<int16_t> mag {StatType::MAG};
    Stat<int16_t> agi {StatType::AGI};
    Stat<int16_t> con {StatType::CON};

    std::unordered_map<StatType, Stat<int16_t>> optionals {};
};

template <std::integral T>
boost::asio::awaitable<void> Stat<T>::set_and_update(boost::mysql::pooled_connection& conn, const T value, const uint32_t character_id) {
    _value = value;
    co_await update(conn, character_id);
}

template <std::integral T>
boost::asio::awaitable<void> Stat<T>::update(boost::mysql::pooled_connection& conn, uint32_t character_id) {
    std::string query {
        format_sql(
            conn->format_opts().value(),
            "UPDATE character_stats"
            "SET {} = {}"
            "WHERE character_id = {}",
            type_to_string(type),
            _value,
            character_id
        )
    };
    boost::mysql::results result;
    co_await conn->async_execute(std::move(query), result, boost::asio::use_awaitable);
}

std::string_view type_to_string(const StatType type) {
    static const std::unordered_map<StatType, const char*> _map {
        {StatType::STR, "str"},
        {StatType::MAG, "mag"},
        {StatType::AGI, "agi"},
        {StatType::CON, "con"},
        {StatType::FAITH, "faith"},
    };

    return _map.at(type);
}
}
