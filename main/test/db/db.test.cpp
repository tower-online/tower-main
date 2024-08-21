#include <catch2/catch_test_macros.hpp>
#include <pqxx/pqxx>
#include <tower/net/db.hpp>
#include <tower/system/settings.hpp>

#include <iostream>

using namespace tower;

TEST_CASE("pqxx connects to database", "[pqxx]") {
    // postgresql://{db_user}:{db_password}@{db_host}:{db_port}/{db_name}
    const std::string option = std::format("postgresql://{}:{}@{}:{}/{}",
        Settings::db_user(), Settings::db_password(), Settings::db_host(), Settings::db_port(), Settings::db_name());

    pqxx::connection connection {option};
    REQUIRE(connection.is_open());

    pqxx::work tx {connection};
    pqxx::row r = tx.exec1("SELECT 1");
    tx.commit();

    connection.close();
    REQUIRE(!connection.is_open());
}

TEST_CASE("Get connection and query to database", "[DB]") {
    boost::asio::io_context ctx {};
    std::vector<std::thread> threads;
    const size_t num_threads = 10;

    for (auto i {0}; i < num_threads; ++i) {
        co_spawn(ctx, []()->boost::asio::awaitable<void> {
            const auto connection_guard = DB::get_connection_guard();
            pqxx::work tx {*connection_guard.connection()};

            auto [id, username] = tx.query1<int, std::string_view>("SELECT id, username FROM users");

            co_return;
        }, boost::asio::detached);
    }

    for (auto i {0}; i < num_threads; ++i) {
        threads.emplace_back([&ctx] {
            ctx.run();
        });
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
}
