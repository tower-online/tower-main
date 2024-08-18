#pragma once

#include <cstdint>
#include <chrono>
#include <string>

namespace tower {
using namespace std::chrono;

class Settings final {
public:
    static void init();

    static uint16_t main_listen_port() { return _main_listen_port; }
    static milliseconds main_tick_interval() { return _main_tick_interval; }

    static std::string_view auth_jwt_key() { return _auth_jwt_key; }

    static std::string_view db_user() { return _db_user; }
    static std::string_view db_password() { return _db_password; }
    static std::string_view db_host() { return _db_user; }
    static std::string_view db_name() { return _db_name; }
    static uint16_t db_port() { return _db_port; }

    static std::string_view redis_host() { return _redis_host; }
    static std::string_view redist_password() { return _redis_password; }

private:
    static std::string read_file(std::string_view path);

    inline static uint16_t _main_listen_port;
    inline static milliseconds _main_tick_interval;

    inline static std::string _auth_jwt_key;
    inline static std::string _auth_jwt_algorithm;

    inline static std::string _db_user;
    inline static std::string _db_password;
    inline static std::string _db_host;
    inline static std::string _db_name;
    inline static uint16_t _db_port;

    inline static std::string _redis_host;
    inline static std::string _redis_password;
};
}
