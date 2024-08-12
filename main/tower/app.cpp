#include <tower/net/server.hpp>

#include <thread>

int main(int argc, char *argv[]) {
    using namespace tower;

#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    const auto nthreads = std::thread::hardware_concurrency();

    net::Server server {};
    server.start();

    spdlog::info("Terminating...");
    return EXIT_SUCCESS;
}
