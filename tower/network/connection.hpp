#pragma once

#include <boost/asio.hpp>
#include <boost/core/noncopyable.hpp>
#include <flatbuffers/flatbuffers.h>

#include <chrono>

namespace tower::network {
using namespace std::chrono;
using boost::asio::ip::tcp;

class Connection final : boost::noncopyable {
public:
    Connection(boost::asio::strand<boost::asio::any_io_executor>&& strand, tcp::socket&& socket,
               std::function<void(std::vector<uint8_t>&&)>&& packet_received, std::function<void()>&& disconnected);
    ~Connection();

    void open(boost::asio::any_io_executor& executor);
    boost::asio::awaitable<bool> connect(const tcp::endpoint& remote_endpoint);
    void disconnect();
    void send_packet(std::shared_ptr<flatbuffers::DetachedBuffer> buffer);

    bool is_connected() const { return _is_connected; }
    std::string remote_endpoint_string() {
        return std::format("{}:{}", _socket.remote_endpoint().address().to_string(), _socket.remote_endpoint().port());
    }

private:
    boost::asio::awaitable<void> receive_packet();

    tcp::socket _socket;
    std::atomic<bool> _is_connected;
    std::function<void(std::vector<uint8_t>&&)> _packet_received;
    std::function<void()> _disconnected;

    boost::asio::strand<boost::asio::any_io_executor> _strand;
};
}
