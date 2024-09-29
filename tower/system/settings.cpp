#include <tower/system/settings.hpp>

#include <cstdlib>
#include <fstream>

namespace tower {
void Settings::init() {
    _listen_port = std::stoi(std::getenv("TOWER_MAIN_LISTEN_PORT"));
    _listen_backlog = std::stoi(std::getenv("LISTEN_BACKLOG"));
    _tick_interval = milliseconds {std::stoi(std::getenv("TICK_INTERVAL"))};

    _auth_jwt_key = read_file(std::getenv("TOWER_AUTH_JWT_KEY_FILE"));

    _db_user = std::getenv("TOWER_DB_USER");
    _db_password = read_file(std::getenv("TOWER_DB_PASSWORD_FILE"));
    _db_host = std::getenv("TOWER_DB_HOST");
    _db_name = std::getenv("TOWER_DB_NAME");
    _db_port = std::stoi(std::getenv("TOWER_DB_PORT"));

    _redis_host = std::getenv("TOWER_REDIS_HOST");
    _redis_password = read_file(std::getenv("TOWER_REDIS_PASSWORD_FILE"));
}

std::string Settings::read_file(std::string_view path) {
    std::ifstream file {path.data()};
    if (!file.is_open()) return {};

    std::string value;
    std::getline(file, value);
    return value;
}
}
