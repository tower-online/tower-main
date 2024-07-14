
#include <spire/net/server.hpp>

int main() {
    using namespace spire;

#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    net::Server server {};
    server.start();

    return 0;
}
