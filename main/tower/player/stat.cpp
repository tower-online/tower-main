#include <tower/network/db.hpp>
#include <tower/player/stat.hpp>

namespace tower::player {
std::optional<int> Stat::get(const Type type) const {
    if (!_stats.contains(type)) return {};
    return _stats.at(type);
}

void Stat::add(const Type type, const int value) {
    if (!_stats.contains(type)) return;
    _stats[type] += value;
}

boost::asio::awaitable<void> Stat::add_update(const Type type, const int value) {
    add(type, value);

    auto conn = co_await DB::get_connection();
    const std::string query {format_sql(
        conn->format_opts().value(),
        "UPDATE character_stats"
        "SET {} = {}"
        "WHERE character_id = {}",
        stat_name,
        value,
        id
    )};
    boost::mysql::results result;
    const auto [ec] = co_await conn->async_execute(query, result, as_tuple(boost::asio::use_awaitable));
}

void Stat::set(const Type type, const int value) {
    _stats.insert_or_assign(type, value);
}

boost::asio::awaitable<void> Stat::set_update(const Type type, const int value) {
    set(type, value);
}
}
