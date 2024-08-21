#pragma once

#include <boost/asio.hpp>
#include <pqxx/pqxx>
#include <tower/system/container/concurrent_queue.hpp>

namespace tower {
class DB final {
public:
    class ConnectionGuard {
    public:
        explicit ConnectionGuard(std::unique_ptr<pqxx::connection> connection)
            : _connection {std::move(connection)} {}

        ~ConnectionGuard() {
            if (_connections_current_count > _connections_initial_count) {
                // Dispose the excessive connection
                _connections_current_count -= 1;
                // Here, current count can be lower than initial count.
                // But it doesn't matter because new connections will be created anyway.
                return;
            }

            _connections.push(std::move(_connection));
        }

        pqxx::connection* connection() const { return _connection.get(); }

    private:
        std::unique_ptr<pqxx::connection> _connection;
    };

    static void init(unsigned num_connections, std::string_view option);
    static void uninit();

    static ConnectionGuard get_connection_guard();


private:
    inline static ConcurrentQueue<std::unique_ptr<pqxx::connection>> _connections {};
    inline static unsigned _connections_initial_count {0};
    inline static std::atomic<unsigned> _connections_current_count {0};
    inline static std::string _option;
};


inline void DB::init(const unsigned num_connections, const std::string_view option) {
    for (auto i {0}; i < num_connections; ++i) {
        _connections.push(std::make_unique<pqxx::connection>(option.data()));
    }
    _connections_initial_count = num_connections;
    _connections_current_count = num_connections;

    _option = std::string {option};
}

inline void DB::uninit() {
    _connections.clear();
}

inline DB::ConnectionGuard DB::get_connection_guard() {
    std::unique_ptr<pqxx::connection> connection;
    if (!_connections.try_pop(connection)) {
        connection = std::make_unique<pqxx::connection>(_option);
        _connections_current_count += 1;
    }

    return ConnectionGuard {std::move(connection)};
}
}
