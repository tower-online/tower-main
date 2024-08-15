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

    // const auto nthreads = std::thread::hardware_concurrency();
    Settings::init();

    setenv("PYTHONPATH", TOWER_LIB_PYTHONPATH, true);
    Py_Initialize();
    if (!JWT::init()) {
        EXIT_FAILURE;
    }

    net::Server server {};
    server.start();

    spdlog::info("Terminating...");
    return EXIT_SUCCESS;
}
