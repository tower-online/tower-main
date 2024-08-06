#include <tower/net/server.hpp>

int main() {
    using namespace tower;

#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    net::Server server {};
    server.start();

    return 0;
}
