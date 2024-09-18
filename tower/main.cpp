#include <boost/redis/src.hpp>
#include <tower/network/server.hpp>
#include <tower/system/settings.hpp>

#include <cstdlib>
#include <iostream>
#include <thread>

int main(int argc, char* argv[]) {
    using namespace tower;

#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("spdlog set level: {}", to_string_view(spdlog::get_level()));
#endif

    Settings::init();

    constexpr size_t num_threads = 4; //std::thread::hardware_concurrency();
    boost::asio::thread_pool workers {num_threads - 1};
    boost::asio::signal_set signals {workers.get_executor(), SIGINT, SIGTERM};

    // SharedState
    boost::mysql::pool_params db_params {};
    db_params.server_address.emplace_host_and_port(Settings::db_host().data(), Settings::db_port());
    db_params.username = Settings::db_user();
    db_params.password = Settings::db_password();
    db_params.database = Settings::db_name();

    // boost::redis::config redis_config {};
    // redis_config.addr.host = Settings::redis_host();
    // redis_config.password = Settings::redis_password();

    auto shared_st = std::make_shared<network::ServerSharedState>(
        boost::mysql::connection_pool {
            boost::mysql::pool_executor_params::thread_safe(workers.get_executor()), std::move(db_params)
        },
        boost::redis::connection {workers.get_executor()}
    );
    shared_st->db_pool.async_run(boost::asio::detached);
    // shared_st->redis_connection.async_run(redis_config, {}, boost::asio::detached);

    // Server
    std::unique_ptr<network::Server> server;
    try {
        server = std::make_unique<network::Server>(workers.get_executor(), shared_st);
    } catch (const boost::system::system_error& e) {
        std::cerr << e.code().message() << std::endl;
        return EXIT_FAILURE;
    }
    server->init();
    server->start();

    signals.async_wait([shared_st, &workers, &server](boost::system::error_code, int) {
        server->stop();
        shared_st->db_pool.cancel();
        workers.stop();
    });

    workers.attach();
    workers.join();

    std::cout << "main exiting..." << std::endl;
    return EXIT_SUCCESS;
}
