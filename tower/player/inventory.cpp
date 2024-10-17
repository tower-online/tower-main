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
        std::string query {
            format_sql(
                conn->format_opts().value(),
                "SELECT item_type "
                "FROM character_items "
                "WHERE character_id = {}",
                character_id
            )
        };

        boost::mysql::results results;
        if (const auto [ec] = co_await conn->async_execute(
            std::move(query), results, as_tuple(boost::asio::use_awaitable)); ec) {
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
        std::string query {
            format_sql(
                conn->format_opts().value(),
                "SELECT damage "
                "FROM item_fist "
                "WHERE character_id = {}",
                character_id
            )
        };

        boost::mysql::results results;
        if (const auto [ec] = co_await conn->async_execute(
            std::move(query), results, as_tuple(boost::asio::use_awaitable)); ec) {
            co_return nullptr;
        }

        if (results.rows().empty()) co_return nullptr;
        const auto& r {results.rows().at(0)};

        auto fist {Fist::create()};
        fist->melee_attack_damage = static_cast<int>(r.at(0).as_int64());

        item = std::move(fist);
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

        std::string query {
            format_sql(
                conn->format_opts().value(),
                "INSERT INTO item_fist (character_id, damage) "
                "VALUES ({}, {})"
                "ON DUPLICATE KEY UPDATE "
                "damage = VALUES(damage) ",
                character_id,
                fist->melee_attack_damage
            )
        };

        boost::mysql::results results;
        if (const auto [ec] = co_await conn->async_execute(
            std::move(query), results, as_tuple(boost::asio::use_awaitable)); ec) {
            co_return false;
        }
    } else {
        co_return false;
    }

    co_return true;
}

boost::asio::awaitable<std::optional<int>> Inventory::load_gold(
    boost::mysql::pooled_connection& conn, const uint32_t character_id) {
    std::string query {
        format_sql(
            conn->format_opts().value(),
            "SELECT amount "
            "FROM character_golds "
            "WHERE character_id = {}",
            character_id
        )
    };

    boost::mysql::results results;
    if (const auto [ec] = co_await conn->async_execute(
        std::move(query), results, as_tuple(boost::asio::use_awaitable)); ec) {
        co_return std::optional<int> {};
    }

    if (results.rows().empty()) co_return std::optional<int> {};

    const int gold {static_cast<int>(results.rows()[0].at(0).as_int64())};
    co_return std::optional<int> {gold};
}

boost::asio::awaitable<bool> Inventory::save_gold(
    boost::mysql::pooled_connection& conn, const uint32_t character_id, const int amount) {
    std::string query {
        format_sql(
            conn->format_opts().value(),
            "UPDATE character_golds "
            "SET amount = {} "
            "WHERE character_id = {}",
            amount,
            character_id
        )
    };

    boost::mysql::results results;
    if (const auto [ec] = co_await conn->async_execute(
        std::move(query), results, as_tuple(boost::asio::use_awaitable)); ec) {
        co_return false;
    }

    co_return true;
}
}
