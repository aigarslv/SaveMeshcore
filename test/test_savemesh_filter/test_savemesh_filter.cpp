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

// ── Protected command validation ────────────────────────────────────────────

static savemesh::ProtectedCheck check(const char* command, char* canonical, size_t cap = 64) {
  canonical[0] = 0;
  return savemesh::checkProtectedSet(command, canonical, cap);
}

TEST(ProtectedCommands, ParseUnsignedIsStrict) {
  uint16_t v = 999;
  EXPECT_TRUE(savemesh::parseUnsigned("0", &v));     EXPECT_EQ(v, 0);
  EXPECT_TRUE(savemesh::parseUnsigned("168", &v));   EXPECT_EQ(v, 168);
  EXPECT_TRUE(savemesh::parseUnsigned("65535", &v)); EXPECT_EQ(v, 65535);
  EXPECT_TRUE(savemesh::parseUnsigned("7  ", &v));   EXPECT_EQ(v, 7);
  EXPECT_TRUE(savemesh::parseUnsigned("7\r", &v));   EXPECT_EQ(v, 7);   // stray line ending
  EXPECT_TRUE(savemesh::parseUnsigned("7\t\n", &v)); EXPECT_EQ(v, 7);
  for (const char* bad : {"", " ", "abc", "-1", "+1", "1x", "1 2", "0x10", "65536", "99999999999999"}) {
    EXPECT_FALSE(savemesh::parseUnsigned(bad, &v)) << "'" << bad << "'";
  }
}

TEST(ProtectedCommands, MatchCommandRequiresWholeWord) {
  const char* arg = nullptr;
  EXPECT_TRUE(savemesh::matchCommand("set multi.acks 1", "set multi.acks", &arg));
  EXPECT_STREQ(arg, "1");
  EXPECT_TRUE(savemesh::matchCommand("set multi.acks     1", "set multi.acks", &arg));
  EXPECT_STREQ(arg, "1");
  EXPECT_TRUE(savemesh::matchCommand("set multi.acks", "set multi.acks", &arg));
  EXPECT_STREQ(arg, "");
  EXPECT_FALSE(savemesh::matchCommand("set multi.acksx 1", "set multi.acks", &arg));
  EXPECT_FALSE(savemesh::matchCommand("xset multi.acks 1", "set multi.acks", &arg));
  EXPECT_FALSE(savemesh::matchCommand("set multi", "set multi.acks", &arg));
}

TEST(ProtectedCommands, UnrelatedCommandsAreNotMatched) {
  char out[64];
  for (const char* cmd : {"set name Node", "get flood.advert.interval", "set flood.max 10",
                          "set advert.intervalx 0", "set flood.advert.interval.x 47", "filter data", ""}) {
    EXPECT_FALSE(check(cmd, out).matched) << cmd;
  }
}

TEST(ProtectedCommands, ExtraSpacesNeverChangeTheStoredValue) {
  // Regression: the stock number parser reads "  47" (two spaces) as 0, which would
  // silently disable flood adverts. The forwarded command is always rebuilt.
  char out[64];
  for (const char* cmd : {"set flood.advert.interval 47", "set flood.advert.interval  47",
                          "set flood.advert.interval     47", "set flood.advert.interval 47  ",
                          "set flood.advert.interval 47\r"}) {
    auto r = check(cmd, out);
    EXPECT_TRUE(r.matched && r.allowed) << cmd;
    EXPECT_STREQ(out, "set flood.advert.interval 47") << cmd;
  }
  auto r = check("set flood.advert.interval   168", out);
  EXPECT_TRUE(r.allowed);
  EXPECT_STREQ(out, "set flood.advert.interval 168");
  r = check("set flood.advert.interval  0", out);
  EXPECT_TRUE(r.allowed);
  EXPECT_STREQ(out, "set flood.advert.interval 0");
}

TEST(ProtectedCommands, FloodAdvertIntervalLimits) {
  char out[64];
  for (const char* ok : {"0", "47", "48", "100", "168"}) {
    char cmd[64]; snprintf(cmd, sizeof cmd, "set flood.advert.interval %s", ok);
    EXPECT_TRUE(check(cmd, out).allowed) << cmd;
  }
  for (const char* bad : {"1", "3", "46", "169", "255", "256", "-1", "abc", ""}) {
    char cmd[64]; snprintf(cmd, sizeof cmd, "set flood.advert.interval %s", bad);
    auto r = check(cmd, out);
    EXPECT_TRUE(r.matched) << cmd;
    EXPECT_FALSE(r.allowed) << cmd;
    EXPECT_EQ(out[0], 0) << "no canonical command may be produced for: " << cmd;
  }
}

TEST(ProtectedCommands, OtherSettingLimits) {
  char out[64];
  EXPECT_FALSE(check("set path.hash.mode 0", out).allowed);
  EXPECT_FALSE(check("set path.hash.mode 3", out).allowed);
  EXPECT_FALSE(check("set path.hash.mode", out).allowed);
  EXPECT_TRUE(check("set path.hash.mode  2", out).allowed);
  EXPECT_STREQ(out, "set path.hash.mode 2");
  EXPECT_TRUE(check("set path.hash.mode 1", out).allowed);

  EXPECT_FALSE(check("set multi.acks 0", out).allowed);
  EXPECT_FALSE(check("set multi.acks 2", out).allowed);
  EXPECT_TRUE(check("set multi.acks   1", out).allowed);
  EXPECT_STREQ(out, "set multi.acks 1");

  EXPECT_FALSE(check("set advert.interval 60", out).allowed);
  EXPECT_FALSE(check("set advert.interval 2", out).allowed);
  EXPECT_TRUE(check("set advert.interval  0", out).allowed);
  EXPECT_STREQ(out, "set advert.interval 0");
}

TEST(ProtectedCommands, LoopDetectAcceptsOnlyWholeWords) {
  char out[64];
  for (const char* word : {"minimal", "moderate", "strict"}) {
    char cmd[64]; snprintf(cmd, sizeof cmd, "set loop.detect  %s", word);   // two spaces
    EXPECT_TRUE(check(cmd, out).allowed) << cmd;
    char expected[64]; snprintf(expected, sizeof expected, "set loop.detect %s", word);
    EXPECT_STREQ(out, expected);
  }
  for (const char* bad : {"off", "minimal-junk", "minimalx", "min", "STRICT", "0", ""}) {
    char cmd[64]; snprintf(cmd, sizeof cmd, "set loop.detect %s", bad);
    auto r = check(cmd, out);
    EXPECT_TRUE(r.matched) << cmd;
    EXPECT_FALSE(r.allowed) << cmd;
  }
}

TEST(ProtectedCommands, CanonicalCommandNeverOverflowsItsBuffer) {
  char out[8];   // deliberately too small: output must be truncated and NUL-terminated
  memset(out, 'x', sizeof out);
  auto r = savemesh::checkProtectedSet("set flood.advert.interval 47", out, sizeof out);
  EXPECT_TRUE(r.allowed);
  EXPECT_EQ(out[sizeof out - 1], 0);
}

TEST(ProtectedCommands, ReportsWhichSettingMatched) {
  char out[64];
  EXPECT_EQ(check("set flood.advert.interval 0", out).key, savemesh::ProtectedKey::FLOOD_ADVERT_INTERVAL);
  EXPECT_EQ(check("set advert.interval 0", out).key, savemesh::ProtectedKey::ADVERT_INTERVAL);
  EXPECT_EQ(check("set loop.detect strict", out).key, savemesh::ProtectedKey::LOOP_DETECT);
  EXPECT_EQ(check("set multi.acks 1", out).key, savemesh::ProtectedKey::MULTI_ACKS);
  EXPECT_EQ(check("set path.hash.mode 1", out).key, savemesh::ProtectedKey::PATH_HASH_MODE);
  EXPECT_EQ(check("set name x", out).key, savemesh::ProtectedKey::NONE);
}

TEST(PublicChannelFilter, BuiltInSelfTestPasses) {
  // Same check the repeater runs for "filter selftest" (software HMAC here).
  EXPECT_TRUE(publicChannelSelfTest());
  EXPECT_STREQ(publicChannelHmacBackend(), "software");
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}