#pragma once

#include <boost/mysql.hpp>
#include <boost/redis.hpp>
#include <tower/game/party.hpp>

namespace tower::network {
struct ServerSharedState {
    boost::mysql::connection_pool db_pool;
    boost::redis::connection redis_connection;
    game::PartyManager party_manager;
};
}