#pragma once

#include <spire/net/client.hpp>

namespace spire::net {
class Client;

struct Packet {
    std::shared_ptr<Client> client;
    std::vector<uint8_t> buffer;

    Packet() = default;
    Packet(std::shared_ptr<Client> client, std::vector<uint8_t>&& buffer)
        : client(std::move(client)), buffer(std::move(buffer)) {}

    Packet(Packet&& other) noexcept
        : client(std::move(other.client)), buffer(std::move(other.buffer)) {}

    Packet& operator=(Packet&& other) noexcept {
        if (this != &other) {
            client = std::move(other.client);
            buffer = std::move(other.buffer);
        }
        return *this;
    }

    Packet(const Packet&) = delete;
    Packet& operator=(const Packet&) = delete;
};
}
