// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_PACKET_SPIRE_NET_PACKET_H_
#define FLATBUFFERS_GENERATED_PACKET_SPIRE_NET_PACKET_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 24 &&
              FLATBUFFERS_VERSION_MINOR == 3 &&
              FLATBUFFERS_VERSION_REVISION == 25,
             "Non-compatible flatbuffers version included");

#include "client_join.hpp"
#include "entity_transform_update.hpp"
#include "spawn_player.hpp"

namespace spire {
namespace net {
namespace packet {

struct Packet;
struct PacketBuilder;

enum class PacketType : uint8_t {
  NONE = 0,
  ClientJoin = 1,
  EntityTransformUpdate = 2,
  SpawnPlayer = 3,
  MIN = NONE,
  MAX = SpawnPlayer
};

inline const PacketType (&EnumValuesPacketType())[4] {
  static const PacketType values[] = {
    PacketType::NONE,
    PacketType::ClientJoin,
    PacketType::EntityTransformUpdate,
    PacketType::SpawnPlayer
  };
  return values;
}

inline const char * const *EnumNamesPacketType() {
  static const char * const names[5] = {
    "NONE",
    "ClientJoin",
    "EntityTransformUpdate",
    "SpawnPlayer",
    nullptr
  };
  return names;
}

inline const char *EnumNamePacketType(PacketType e) {
  if (::flatbuffers::IsOutRange(e, PacketType::NONE, PacketType::SpawnPlayer)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesPacketType()[index];
}

template<typename T> struct PacketTypeTraits {
  static const PacketType enum_value = PacketType::NONE;
};

template<> struct PacketTypeTraits<spire::net::packet::ClientJoin> {
  static const PacketType enum_value = PacketType::ClientJoin;
};

template<> struct PacketTypeTraits<spire::net::packet::EntityTransformUpdate> {
  static const PacketType enum_value = PacketType::EntityTransformUpdate;
};

template<> struct PacketTypeTraits<spire::net::packet::SpawnPlayer> {
  static const PacketType enum_value = PacketType::SpawnPlayer;
};

bool VerifyPacketType(::flatbuffers::Verifier &verifier, const void *obj, PacketType type);
bool VerifyPacketTypeVector(::flatbuffers::Verifier &verifier, const ::flatbuffers::Vector<::flatbuffers::Offset<void>> *values, const ::flatbuffers::Vector<PacketType> *types);

struct Packet FLATBUFFERS_FINAL_CLASS : private ::flatbuffers::Table {
  typedef PacketBuilder Builder;
  struct Traits;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_PACKET_TYPE = 4,
    VT_PACKET = 6
  };
  spire::net::packet::PacketType packet_type() const {
    return static_cast<spire::net::packet::PacketType>(GetField<uint8_t>(VT_PACKET_TYPE, 0));
  }
  const void *packet() const {
    return GetPointer<const void *>(VT_PACKET);
  }
  template<typename T> const T *packet_as() const;
  const spire::net::packet::ClientJoin *packet_as_ClientJoin() const {
    return packet_type() == spire::net::packet::PacketType::ClientJoin ? static_cast<const spire::net::packet::ClientJoin *>(packet()) : nullptr;
  }
  const spire::net::packet::EntityTransformUpdate *packet_as_EntityTransformUpdate() const {
    return packet_type() == spire::net::packet::PacketType::EntityTransformUpdate ? static_cast<const spire::net::packet::EntityTransformUpdate *>(packet()) : nullptr;
  }
  const spire::net::packet::SpawnPlayer *packet_as_SpawnPlayer() const {
    return packet_type() == spire::net::packet::PacketType::SpawnPlayer ? static_cast<const spire::net::packet::SpawnPlayer *>(packet()) : nullptr;
  }
  bool Verify(::flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint8_t>(verifier, VT_PACKET_TYPE, 1) &&
           VerifyOffset(verifier, VT_PACKET) &&
           VerifyPacketType(verifier, packet(), packet_type()) &&
           verifier.EndTable();
  }
};

template<> inline const spire::net::packet::ClientJoin *Packet::packet_as<spire::net::packet::ClientJoin>() const {
  return packet_as_ClientJoin();
}

template<> inline const spire::net::packet::EntityTransformUpdate *Packet::packet_as<spire::net::packet::EntityTransformUpdate>() const {
  return packet_as_EntityTransformUpdate();
}

template<> inline const spire::net::packet::SpawnPlayer *Packet::packet_as<spire::net::packet::SpawnPlayer>() const {
  return packet_as_SpawnPlayer();
}

struct PacketBuilder {
  typedef Packet Table;
  ::flatbuffers::FlatBufferBuilder &fbb_;
  ::flatbuffers::uoffset_t start_;
  void add_packet_type(spire::net::packet::PacketType packet_type) {
    fbb_.AddElement<uint8_t>(Packet::VT_PACKET_TYPE, static_cast<uint8_t>(packet_type), 0);
  }
  void add_packet(::flatbuffers::Offset<void> packet) {
    fbb_.AddOffset(Packet::VT_PACKET, packet);
  }
  explicit PacketBuilder(::flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ::flatbuffers::Offset<Packet> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = ::flatbuffers::Offset<Packet>(end);
    return o;
  }
};

inline ::flatbuffers::Offset<Packet> CreatePacket(
    ::flatbuffers::FlatBufferBuilder &_fbb,
    spire::net::packet::PacketType packet_type = spire::net::packet::PacketType::NONE,
    ::flatbuffers::Offset<void> packet = 0) {
  PacketBuilder builder_(_fbb);
  builder_.add_packet(packet);
  builder_.add_packet_type(packet_type);
  return builder_.Finish();
}

struct Packet::Traits {
  using type = Packet;
  static auto constexpr Create = CreatePacket;
};

inline bool VerifyPacketType(::flatbuffers::Verifier &verifier, const void *obj, PacketType type) {
  switch (type) {
    case PacketType::NONE: {
      return true;
    }
    case PacketType::ClientJoin: {
      auto ptr = reinterpret_cast<const spire::net::packet::ClientJoin *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case PacketType::EntityTransformUpdate: {
      auto ptr = reinterpret_cast<const spire::net::packet::EntityTransformUpdate *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case PacketType::SpawnPlayer: {
      auto ptr = reinterpret_cast<const spire::net::packet::SpawnPlayer *>(obj);
      return verifier.VerifyTable(ptr);
    }
    default: return true;
  }
}

inline bool VerifyPacketTypeVector(::flatbuffers::Verifier &verifier, const ::flatbuffers::Vector<::flatbuffers::Offset<void>> *values, const ::flatbuffers::Vector<PacketType> *types) {
  if (!values || !types) return !values && !types;
  if (values->size() != types->size()) return false;
  for (::flatbuffers::uoffset_t i = 0; i < values->size(); ++i) {
    if (!VerifyPacketType(
        verifier,  values->Get(i), types->GetEnum<PacketType>(i))) {
      return false;
    }
  }
  return true;
}

inline const spire::net::packet::Packet *GetPacket(const void *buf) {
  return ::flatbuffers::GetRoot<spire::net::packet::Packet>(buf);
}

inline const spire::net::packet::Packet *GetSizePrefixedPacket(const void *buf) {
  return ::flatbuffers::GetSizePrefixedRoot<spire::net::packet::Packet>(buf);
}

inline bool VerifyPacketBuffer(
    ::flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<spire::net::packet::Packet>(nullptr);
}

inline bool VerifySizePrefixedPacketBuffer(
    ::flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<spire::net::packet::Packet>(nullptr);
}

inline void FinishPacketBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<spire::net::packet::Packet> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedPacketBuffer(
    ::flatbuffers::FlatBufferBuilder &fbb,
    ::flatbuffers::Offset<spire::net::packet::Packet> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace packet
}  // namespace net
}  // namespace spire

#endif  // FLATBUFFERS_GENERATED_PACKET_SPIRE_NET_PACKET_H_
