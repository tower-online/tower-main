#include <boost/python.hpp>
#include <boost/redis/src.hpp>
#include <tower/net/server.hpp>
#include <tower/system/settings.hpp>

#include <cstdlib>
#include <Python.h>
#include <thread>

int main(int argc, char *argv[]) {
    using namespace tower;
    namespace python = boost::python;

#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    Py_Initialize();

    try {
        python::object jwt_module = python::import("jwt");
    }
    catch (const python::error_already_set&) {
        PyErr_Print();
        return EXIT_FAILURE;
    }
    catch (const std::exception& e) {
        spdlog::error("Error importing python module: {}", e.what());
        return EXIT_FAILURE;
    }


    // const auto nthreads = std::thread::hardware_concurrency();
    Settings::init();

    net::Server server {};
    server.start();

    spdlog::info("Terminating...");
    return EXIT_SUCCESS;
}
