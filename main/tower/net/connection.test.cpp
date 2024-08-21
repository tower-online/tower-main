#include <catch2/catch_test_macros.hpp>
#include <tower/net/connection.hpp>
#include <tower/net/packet/packet_base.hpp>

using namespace tower::net;
using namespace tower::net::packet;

TEST_CASE("flatbuffers verifies PacketBase and contents", "[Connection]") {
    flatbuffers::FlatBufferBuilder builder {256};

    const auto request = CreateClientJoinRequestDirect(builder, ClientPlatform::TEST, "Foo", "my_precious_token");
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::ClientJoinRequest, request.Union()));

    flatbuffers::Verifier verifier {builder.GetBufferPointer(), builder.GetSize()};
    REQUIRE(VerifySizePrefixedPacketBaseBuffer(verifier));

    const auto packet_base = GetSizePrefixedPacketBase(builder.GetBufferPointer());
    REQUIRE(packet_base != nullptr);

    REQUIRE(VerifySizePrefixedClientJoinRequestBuffer(verifier));

    const auto deserialized_request = packet_base->packet_base_as<ClientJoinRequest>();
    REQUIRE(deserialized_request->Verify(verifier));
}

TEST_CASE("flatbuffers verifies invalid PacketBase and contents", "[Connection]") {
    flatbuffers::FlatBufferBuilder builder {256};

    const auto request = CreateClientJoinRequestDirect(builder, ClientPlatform::TEST, "Foo", "my_precious_token");
    builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::ClientJoinResponse, request.Union()));

    flatbuffers::Verifier verifier {builder.GetBufferPointer(), builder.GetSize()};
    REQUIRE(VerifySizePrefixedPacketBaseBuffer(verifier));

    const auto packet_base = GetSizePrefixedPacketBase(builder.GetBufferPointer());
    REQUIRE(packet_base != nullptr);

    const auto deserialized_request = packet_base->packet_base_as<ClientJoinRequest>();
    REQUIRE(!deserialized_request->Verify(verifier));
}