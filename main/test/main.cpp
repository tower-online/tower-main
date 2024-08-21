#include <catch2/catch_session.hpp>
#include <tower/net/db.hpp>
#include <tower/system/settings.hpp>

int main(int argc, char* argv[]) {
    using namespace tower;

    // init
    Settings::init();
    DB::init(2, std::format("postgresql://{}:{}@{}:{}/{}",
        Settings::db_user(), Settings::db_password(), Settings::db_host(), Settings::db_port(), Settings::db_name()));

    int result = Catch::Session().run(argc, argv);

    // uninit

    return result;
}
