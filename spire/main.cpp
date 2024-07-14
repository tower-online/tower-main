#include <spire/game/client.hpp>
#include <spire/net/listener.hpp>
#include <spire/net/packet/packet.hpp>

#include <random>

int main() {
    using namespace spire;
    using boost::asio::ip::tcp;

    std::vector<std::shared_ptr<game::Client>> clients;

    boost::asio::io_context ctx {};
    net::Listener listener {ctx, 30000, [&clients](boost::asio::io_context& ctx, tcp::socket&& socket) {
        static std::atomic<uint32_t> id_generator {0};

        auto client = std::make_shared<game::Client>(ctx, std::move(socket), ++id_generator);
        client->start();

        using namespace net::packet;
        flatbuffers::FlatBufferBuilder builder {64};
        const auto client_join = CreateClientJoin(builder, client->id);
        builder.FinishSizePrefixed(CreatePacket(builder, PacketType::ClientJoin, client_join.Union()));
        client->send_packet(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));

        clients.push_back(std::move(client));


        // static std::random_device rd;
        // static std::mt19937 rng {rd()};
        // static std::uniform_real_distribution<float> distribution {-10.0, 10.0};
        // client->player->position = glm::vec2 {distribution(rng), distribution(rng)};
    }};
    listener.start();

    ctx.run();

    return 0;
}
