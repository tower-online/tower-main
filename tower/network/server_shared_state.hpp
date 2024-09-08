#pragma once

#include <boost/mysql.hpp>
#include <boost/redis.hpp>

namespace tower::network {
struct ServerSharedState {
    boost::mysql::connection_pool db_pool;
    boost::redis::connection redis_connection;
};
}