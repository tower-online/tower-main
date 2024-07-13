#include <spire/game/client.hpp>
#include <spire/net/connection.hpp>
#include <spire/net/listener.hpp>

int main() {
    using namespace spire;
    using boost::asio::ip::tcp;

    std::vector<std::unique_ptr<game::Client>> clients;

    boost::asio::io_context ctx {};
    net::Listener listener {ctx, 30000, [&clients](boost::asio::io_context& ctx, tcp::socket&& socket) {
        static std::atomic<uint32_t> id_generator {0};

        auto client = std::make_unique<game::Client>(ctx, std::move(socket), ++id_generator);
        client->start();
        clients.push_back(std::move(client));
    }};
    listener.start();

    ctx.run();

    return 0;
}
