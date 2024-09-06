#pragma once

#include <boost/asio.hpp>
#include <boost/mysql.hpp>

namespace tower {
class DB final {
public:
    static void init(boost::asio::io_context& ctx, boost::mysql::pool_params&& params);
    static boost::asio::awaitable<boost::mysql::pooled_connection> get_connection();

private:
    inline static std::unique_ptr<boost::mysql::connection_pool> _pool;
};

inline void DB::init(boost::asio::io_context& ctx, boost::mysql::pool_params&& params) {
    _pool = std::make_unique<boost::mysql::connection_pool>(
        boost::mysql::pool_executor_params::thread_safe(ctx.get_executor()), std::move(params));
    _pool->async_run(boost::asio::detached);
}

inline boost::asio::awaitable<boost::mysql::pooled_connection> DB::get_connection() {
    auto connection = co_await _pool->async_get_connection(boost::asio::use_awaitable);
    co_return connection;
}
}
