#include <boost/python.hpp>
#include <boost/redis/src.hpp>
#include <tower/net/server.hpp>
#include <tower/system/jwt.hpp>
#include <tower/system/settings.hpp>

#include <cstdlib>
#include <Python.h>
#include <thread>

int main(int argc, char *argv[]) {
    using namespace tower;

#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    const auto num_threads = std::thread::hardware_concurrency();
    Settings::init();

    // Initialize python module embeddings
    setenv("PYTHONPATH", TOWER_LIB_PYTHONPATH, true);
    spdlog::info("set PYTHONPATH={}", std::getenv("PYTHONPATH"));
    Py_Initialize();
    if (!JWT::init()) {
        EXIT_FAILURE;
    }

    net::Server server {num_threads / 2, num_threads / 2};
    server.start();
    server.join();

    spdlog::info("Terminating...");
    return EXIT_SUCCESS;
}
