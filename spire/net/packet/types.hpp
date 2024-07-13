// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_TYPES_SPIRE_NET_PACKET_H_
#define FLATBUFFERS_GENERATED_TYPES_SPIRE_NET_PACKET_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 24 &&
              FLATBUFFERS_VERSION_MINOR == 3 &&
              FLATBUFFERS_VERSION_REVISION == 25,
             "Non-compatible flatbuffers version included");

namespace spire {
namespace net {
namespace packet {

struct Vector2;

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) Vector2 FLATBUFFERS_FINAL_CLASS {
 private:
  float x_;
  float y_;

 public:
  struct Traits;
  Vector2()
      : x_(0),
        y_(0) {
  }
  Vector2(float _x, float _y)
      : x_(::flatbuffers::EndianScalar(_x)),
        y_(::flatbuffers::EndianScalar(_y)) {
  }
  float x() const {
    return ::flatbuffers::EndianScalar(x_);
  }
  float y() const {
    return ::flatbuffers::EndianScalar(y_);
  }
};
FLATBUFFERS_STRUCT_END(Vector2, 8);

struct Vector2::Traits {
  using type = Vector2;
};

}  // namespace packet
}  // namespace net
}  // namespace spire

#endif  // FLATBUFFERS_GENERATED_TYPES_SPIRE_NET_PACKET_H_
