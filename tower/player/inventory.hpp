#pragma once

#include <boost/asio.hpp>
#include <boost/mysql.hpp>
#include <tower/item/equipment/equipment.hpp>

namespace tower::player {
using namespace tower::item;

class Inventory {
public:
    std::shared_ptr<Equipment>& get_main_weapon();
    void set_main_weapon(std::shared_ptr<Equipment> weapon);

    boost::asio::awaitable<bool> load_inventory(boost::mysql::pooled_connection& conn, uint32_t character_id);

    static boost::asio::awaitable<std::shared_ptr<Item>> load_item(
        boost::mysql::pooled_connection& conn, uint32_t character_id, std::string_view item_type);
    static boost::asio::awaitable<bool> save_item(
        boost::mysql::pooled_connection& conn, uint32_t character_id, const std::shared_ptr<Item>& item);

    static boost::asio::awaitable<std::optional<int>> load_gold(
        boost::mysql::pooled_connection& conn, uint32_t character_id);
    static boost::asio::awaitable<bool> save_gold(
        boost::mysql::pooled_connection& conn, uint32_t character_id, int amount);

private:
    std::shared_ptr<Equipment> _main_weapon;

    uint32_t _golds {};
    std::vector<std::shared_ptr<Item>> _items {};
};
}
