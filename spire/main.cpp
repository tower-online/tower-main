#include <spire/net/listener.hpp>

int main() {
    using namespace spire;
    using boost::asio::ip::tcp;

    boost::asio::io_context ctx {};
    net::Listener listener {ctx, 30000, [](boost::asio::io_context&, tcp::socket&&){}};
    listener.start();

    ctx.run();

    return 0;
}