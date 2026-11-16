#pragma once

#include <Packet.h>

namespace savemesh {

static_assert(PAYLOAD_TYPE_GRP_DATA == 0x06, "Review the data filter for the new GRP_DATA value");
static_assert(PAYLOAD_TYPE_RAW_CUSTOM == 0x0F, "Review the data filter for the new RAW_CUSTOM value");

inline bool shouldDropOneByte(const mesh::Packet* packet, bool enabled) {
  return enabled && packet->isRouteFlood()
      && packet->getPathHashSize() == 1 && packet->getPathHashCount() > 0;
}

inline bool shouldDropData(const mesh::Packet* packet, bool enabled) {
  if (!enabled) return false;
  uint8_t type = packet->getPayloadType();
  // getPayloadType() is a 4-bit value, so this covers reserved 0x0C-0x0E
  // and RAW_CUSTOM 0x0F.
  return type == PAYLOAD_TYPE_GRP_DATA || type >= 0x0C;
}

}
