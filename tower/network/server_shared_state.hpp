#pragma once

#include <boost/asio.hpp>
#include <boost/mysql.hpp>
#include <boost/redis.hpp>
#include <tower/game/party.hpp>

namespace tower::network {
class ServerSharedState {
public:
    ServerSharedState(boost::mysql::connection_pool&& db_pool, boost::redis::connection&& redis_connection,
        boost::asio::any_io_executor&& executor)
        : db_pool {std::move(db_pool)}, redis_connection {std::move(redis_connection)}, server_strand {make_strand(executor)} {}

    boost::mysql::connection_pool db_pool;
    boost::redis::connection redis_connection;
    boost::asio::strand<boost::asio::any_io_executor> server_strand;
    game::PartyManager party_manager {};
};
}