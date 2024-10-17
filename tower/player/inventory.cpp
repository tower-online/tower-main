#include <tower/player/inventory.hpp>
#include <tower/item/equipment/fist.hpp>

namespace tower::player {
std::shared_ptr<Equipment>& Inventory::get_main_weapon() {
    return _main_weapon;
}

void Inventory::set_main_weapon(std::shared_ptr<Equipment> weapon) {
    _main_weapon = weapon;
}

boost::asio::awaitable<bool> Inventory::load_inventory(boost::mysql::pooled_connection& conn, const uint32_t character_id) {
    // Load inventory items
    {
        auto [ec, statement] = co_await conn->async_prepare_statement(
            "SELECT item_type "
            "FROM character_items "
            "WHERE character_id = ?",
        as_tuple(boost::asio::use_awaitable));
        if (ec) co_return false;

        boost::mysql::results results;
        if (const auto [ec] = co_await conn->async_execute(
            statement.bind(character_id), results, as_tuple(boost::asio::use_awaitable)); ec) {
            co_return false;
        }

        for (const auto& row : results.rows()) {
            const auto item_type {row.at(0).as_string()};
            auto item {co_await load_item(conn, character_id, item_type)};
            if (!item) continue;

            _items.push_back(std::move(item));
        }
    }

    const auto golds {co_await load_gold(conn, character_id)};
    if (!golds) co_return false;
    _golds = *golds;

    co_return true;
}

boost::asio::awaitable<std::shared_ptr<Item>> Inventory::load_item(boost::mysql::pooled_connection& conn, uint32_t character_id,
    std::string_view item_type) {
    const auto type {Item::item_name_to_type(item_type)};
    std::shared_ptr<Item> item;

    if (type == ItemType::NONE) co_return nullptr;

    if (type == ItemType::FIST) {
        auto [ec, statement] = co_await conn->async_prepare_statement(
            "SELECT damage "
            "FROM item_fist "
            "WHERE character_id = ?",
            as_tuple(boost::asio::use_awaitable));
        if (ec) co_return nullptr;

        boost::mysql::results results;
        if (const auto [ec] = co_await conn->async_execute(
            statement.bind(character_id), results, as_tuple(boost::asio::use_awaitable)); ec) {
            co_return nullptr;
        }

        if (results.rows().empty()) co_return nullptr;
        const auto& r {results.rows().at(0)};

        auto fist {Fist::create()};
        fist->melee_attack_damage = r.at(0).as_int64();
    } else {
        co_return nullptr;
    }

    co_return item;
}

boost::asio::awaitable<bool> Inventory::save_item(boost::mysql::pooled_connection& conn, uint32_t character_id,
    const std::shared_ptr<Item>& item) {
    if (!item || item->type == ItemType::NONE) co_return false;

    if (item->type == ItemType::FIST) {
        const auto fist {dynamic_cast<const Fist*>(item.get())};

        auto [ec, statement] = co_await conn->async_prepare_statement(
            "INSERT INTO item_fist (character_id, damage) "
            "VALUES (?, ?)"
            "ON DUPLICATE KEY UPDATE "
            "damage = VALUES(damage) ",
            as_tuple(boost::asio::use_awaitable));
        if (ec) co_return false;

        boost::mysql::results results;
        if (const auto [ec] = co_await conn->async_execute(
            statement.bind(character_id, fist->melee_attack_damage), results, as_tuple(boost::asio::use_awaitable)); ec) {
            co_return false;
        }
    } else {
        co_return false;
    }

    co_return true;
}

boost::asio::awaitable<std::optional<int>> Inventory::load_gold(
    boost::mysql::pooled_connection& conn, const uint32_t character_id) {
    auto [ec, statement] = co_await conn->async_prepare_statement(
        "SELECT amount "
        "FROM character_golds "
        "WHERE character_id = ?",
        as_tuple(boost::asio::use_awaitable));
    if (ec) co_return std::optional<int> {};

    boost::mysql::results results;
    if (const auto [ec] = co_await conn->async_execute(
        statement.bind(character_id), results, as_tuple(boost::asio::use_awaitable)); ec) {
        co_return std::optional<int> {};
    }

    if (results.rows().empty()) co_return std::optional<int> {};

    const int gold {static_cast<int>(results.rows()[0].at(0).as_int64())};
    co_return std::optional<int> {gold};
}

boost::asio::awaitable<bool> Inventory::save_gold(
    boost::mysql::pooled_connection& conn, const uint32_t character_id, const int amount) {
    auto [ec, statement] = co_await conn->async_prepare_statement(
        "UPDATE character_golds "
        "SET amount = ? "
        "WHERE character_id = ?",
        as_tuple(boost::asio::use_awaitable));
    if (ec) co_return false;

    boost::mysql::results results;
    if (const auto [ec] = co_await conn->async_execute(
        statement.bind(amount, character_id), results, as_tuple(boost::asio::use_awaitable)); ec) {
        co_return false;
    }

    co_return true;
}
}
