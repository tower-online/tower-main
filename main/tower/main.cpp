#include <boost/redis/src.hpp>
#include <tower/net/server.hpp>
#include <tower/system/settings.hpp>

#include <cstdlib>
#include <thread>

int main(int argc, char *argv[]) {
    using namespace tower;

#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    const unsigned num_threads = std::thread::hardware_concurrency();
    Settings::init();

    net::Server server {num_threads / 2, num_threads / 2};
    server.start();
    server.join();

    spdlog::info("Terminating...");
    return EXIT_SUCCESS;
}
