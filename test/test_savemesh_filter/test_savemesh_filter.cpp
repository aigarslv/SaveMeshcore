#include <gtest/gtest.h>

#include "ForwardFilter.h"
#include "RepeaterPrefsPolicy.h"
#include "ChannelMatch.h"

using namespace mesh;

static Packet packet(uint8_t route, uint8_t type, uint8_t hash_size = 1, uint8_t hops = 0) {
  Packet p{};
  p.header = route | (type << PH_TYPE_SHIFT);
  p.setPathHashSizeAndCount(hash_size, hops);
  return p;
}

TEST(SaveMeshFilter, OneByteDropsOnlyMultiHopFloods) {
  for (uint8_t route = 0; route <= ROUTE_TYPE_TRANSPORT_DIRECT; route++) {
    for (uint8_t hash_size = 1; hash_size <= 4; hash_size++) {
      for (uint8_t hops : {0, 1, 63}) {
        Packet p = packet(route, PAYLOAD_TYPE_TXT_MSG, hash_size, hops);
        bool expected = (route == ROUTE_TYPE_FLOOD || route == ROUTE_TYPE_TRANSPORT_FLOOD)
                     && hash_size == 1 && hops > 0;
        EXPECT_EQ(savemesh::shouldDropOneByte(&p, true), expected);
        EXPECT_FALSE(savemesh::shouldDropOneByte(&p, false));
      }
    }
  }
}

TEST(SaveMeshFilter, DataDropsExactlyConfiguredPayloadTypes) {
  for (uint8_t route = 0; route <= ROUTE_TYPE_TRANSPORT_DIRECT; route++) {
    for (uint8_t type = 0; type <= 0x0F; type++) {
      Packet p = packet(route, type);
      bool expected = type == PAYLOAD_TYPE_GRP_DATA || type >= 0x0C;
      EXPECT_EQ(savemesh::shouldDropData(&p, true), expected);
      EXPECT_FALSE(savemesh::shouldDropData(&p, false));
    }
  }
}

struct TestPrefs {
  uint8_t path_hash_mode;
  uint8_t multi_acks;
  uint8_t loop_detect;
  uint8_t advert_interval;
  uint8_t flood_advert_interval;
};

TEST(RepeaterPrefsPolicy, LeavesAllowedValuesUnchanged) {
  TestPrefs two_byte{1, 1, 1, 0, 47};
  EXPECT_FALSE(savemesh::repair(two_byte));
  TestPrefs three_byte{2, 1, 3, 0, 168};
  EXPECT_FALSE(savemesh::repair(three_byte));
  TestPrefs flood_adverts_disabled{1, 1, 1, 0, 0};
  EXPECT_FALSE(savemesh::repair(flood_adverts_disabled));
}

TEST(RepeaterPrefsPolicy, RepairsEveryStoredByteValue) {
  const TestPrefs base{1, 1, 1, 0, 47};
  for (uint16_t value = 0; value <= UINT8_MAX; value++) {
    TestPrefs p = base;
    p.path_hash_mode = value;
    savemesh::repair(p);
    EXPECT_EQ(p.path_hash_mode, value < 1 ? 1 : value > 2 ? 2 : value);

    p = base;
    p.multi_acks = value;
    savemesh::repair(p);
    EXPECT_EQ(p.multi_acks, 1);

    p = base;
    p.loop_detect = value;
    savemesh::repair(p);
    EXPECT_EQ(p.loop_detect, value < 1 ? 1 : value > 3 ? 3 : value);

    p = base;
    p.advert_interval = value;
    savemesh::repair(p);
    EXPECT_EQ(p.advert_interval, 0);

    p = base;
    p.flood_advert_interval = value;
    savemesh::repair(p);
    EXPECT_EQ(p.flood_advert_interval, value == 0 ? 0 : value < 47 ? 47 : value > 168 ? 168 : value);
  }
}

static Packet publicPacket(const uint8_t* bytes, size_t len, uint8_t type = PAYLOAD_TYPE_GRP_TXT) {
  Packet p = packet(ROUTE_TYPE_FLOOD, type);
  memcpy(p.payload, bytes, len);
  p.payload_len = len;
  return p;
}

TEST(PublicChannelFilter, MatchesFixedRealHmacVectors) {
  // HMAC-SHA256, truncated to two bytes, with the zero-padded Public key.
  const uint8_t one_block[] = {
    0x11, 0x47, 0x0c,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  };
  const uint8_t two_blocks[] = {
    0x11, 0x50, 0x30,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  };
  Packet p1 = publicPacket(one_block, sizeof(one_block));
  Packet p2 = publicPacket(two_blocks, sizeof(two_blocks), PAYLOAD_TYPE_GRP_DATA);
  EXPECT_TRUE(isPublicChannelPacket(&p1));
  EXPECT_TRUE(isPublicChannelPacket(&p2));
}

TEST(PublicChannelFilter, RejectsWrongMacHashTypeAndLengths) {
  const uint8_t valid[] = {
    0x11, 0x47, 0x0c,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  };
  Packet p = publicPacket(valid, sizeof(valid));

  p.payload[1] ^= 1;
  EXPECT_FALSE(isPublicChannelPacket(&p));
  p.payload[1] ^= 1;

  p.payload[0] = 0x12;
  EXPECT_FALSE(isPublicChannelPacket(&p));
  p.payload[0] = 0x11;

  p.header = ROUTE_TYPE_FLOOD | (PAYLOAD_TYPE_TXT_MSG << PH_TYPE_SHIFT);
  EXPECT_FALSE(isPublicChannelPacket(&p));

  p.header = ROUTE_TYPE_FLOOD | (PAYLOAD_TYPE_GRP_TXT << PH_TYPE_SHIFT);
  for (uint8_t len = 0; len < sizeof(valid); len++) {
    p.payload_len = len;
    EXPECT_FALSE(isPublicChannelPacket(&p));
  }

  p.payload_len = sizeof(valid) + 1;
  EXPECT_FALSE(isPublicChannelPacket(&p));
}

TEST(PublicChannelFilter, RouteDoesNotChangeChannelIdentity) {
  const uint8_t valid[] = {
    0x11, 0x47, 0x0c,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  };
  for (uint8_t route = 0; route <= ROUTE_TYPE_TRANSPORT_DIRECT; route++) {
    Packet p = publicPacket(valid, sizeof(valid));
    p.header = route | (PAYLOAD_TYPE_GRP_TXT << PH_TYPE_SHIFT);
    EXPECT_TRUE(isPublicChannelPacket(&p));
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}