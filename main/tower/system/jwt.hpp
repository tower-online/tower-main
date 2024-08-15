#pragma once

#include <boost/python.hpp>
#include <spdlog/spdlog.h>

#include <chrono>
#include <ctime>
#include <map>

namespace tower {
using namespace std::chrono;
namespace py = boost::python;

class JWT final {
public:
    static bool init();

    static std::string encode(std::string_view key, std::string_view algorithm, std::string_view platform, std::string_view username, int expire_hours);
    static bool authenticate(std::string_view token, std::string_view key, std::string_view algorithm,
                             std::string_view platform_expected, std::string_view username_expected);

private:
    inline static py::object _module;
    inline static py::object _encode_func;
    inline static py::object _decode_func;
    inline static py::object _authenticate_func;
};

inline bool JWT::init() {
    try {
        _module = py::import("jwt_wrapper");
        _encode_func = _module.attr("encode");
        _decode_func = _module.attr("decode");
        _authenticate_func = _module.attr("authenticate");
    } catch (const py::error_already_set &) {
        PyErr_Print();
        return false;
    }

    return true;
}

inline std::string JWT::encode(std::string_view key, std::string_view algorithm, std::string_view platform,
                               std::string_view username, int expire_hours){
    const auto token = _encode_func(
        std::string {key}, std::string {algorithm}, std::string{platform}, std::string{username}, expire_hours);
    return py::extract<std::string>(token);
}

inline bool JWT::authenticate(std::string_view token, std::string_view key, std::string_view algorithm,
                              std::string_view platform_expected, std::string_view username_expected) {
    const auto result = _authenticate_func(
        std::string {token}, std::string {key}, std::string {algorithm},
        std::string {platform_expected}, std::string {username_expected});

    return py::extract<bool>(result);
}
}
