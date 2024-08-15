#include <catch2/catch_test_macros.hpp>
#include <tower/system/jwt.hpp>

#include <Python.h>
using namespace tower;

TEST_CASE("JWT module import and call", "[JWT]") {
    setenv("PYTHONPATH", TOWER_LIB_PYTHONPATH, true);
    Py_Initialize();
    JWT::init();

    constexpr std::string_view key = "8dbb9cdeafd154eb956678d277db61c0b9ca85b448d89aa4ad820d1b822b9d62";
    constexpr std::string_view algorithm = "HS256";
    SECTION("JWT encode/authenticate success") {
        const auto token = JWT::encode(key, algorithm, "TEST", "Foo", 1);
        const std::string_view hardcoded_token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJwbGF0Zm9ybSI6IlRFU1QiLCJ1c2VybmFtZSI6IkZvbyIsImV4cCI6MzI1MDM2ODAwMDB9.vkzkie5eY4xmbFXWEEuyVdTvH8Ghhl8HOfOf5xQvxd0";

        REQUIRE(JWT::authenticate(token, key, algorithm, "TEST", "Foo"));
        REQUIRE(JWT::authenticate(hardcoded_token, key, algorithm, "TEST", "Foo"));
    }

    SECTION("JWT authentication fails on the expired token") {
        // Expires on 1970.01.01
        const std::string_view token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJwbGF0Zm9ybSI6IlRFU1QiLCJ1c2VybmFtZSI6IkZvbyIsImV4cCI6MH0.LMiHVvgba4-xRPzr5Cbe0FV4cndGLOtWrPCLrVUU_68";
        REQUIRE(!JWT::authenticate(token, key, algorithm, "TEST", "Foo"));
    }

    SECTION("JWT authentication fails on invalid signature") {
        // Invalid signature
        const std::string_view token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJwbGF0Zm9ybSI6IlRFU1QiLCJ1c2VybmFtZSI6IkZvbyIsImV4cCI6MH0.12345";
        REQUIRE(!JWT::authenticate(token, key, algorithm, "TEST", "Foo"));
    }

    SECTION("JWT authentication fails on invalid platform") {
        // platform=TEST, username=Foo
        const std::string_view token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJwbGF0Zm9ybSI6IlRFU1QiLCJ1c2VybmFtZSI6IkZvbyIsImV4cCI6MH0.LMiHVvgba4-xRPzr5Cbe0FV4cndGLOtWrPCLrVUU_68";
        REQUIRE(!JWT::authenticate(token, key, algorithm, "TEST", "Foo"));
    }

    SECTION("JWT authentication fails on invalid username") {
        // platform=TEST, username=Bar
        const std::string_view token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJwbGF0Zm9ybSI6IlRFU1QiLCJ1c2VybmFtZSI6IkJhciIsImV4cCI6MzI1MDM2ODAwMDB9.-dJWbLaVpXnrgxY7vMEUeOufRCAs1PgWiDcbHDEl7Oc";
        REQUIRE(!JWT::authenticate(token, key, algorithm, "TEST", "Foo"));
    }
}
