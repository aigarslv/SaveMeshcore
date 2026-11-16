#include "ChannelMatch.h"
#include <MeshCore.h>
#include <SHA256.h>
#include <string.h>

namespace {

static_assert(PUB_KEY_SIZE == 32, "Review Public matching for a new key size");
static_assert(CIPHER_MAC_SIZE == 2, "Review Public matching for a new MAC size");
static_assert(CIPHER_BLOCK_SIZE == 16, "Review Public matching for a new cipher block size");
static_assert(PAYLOAD_TYPE_GRP_TXT == 0x05, "Review Public matching for the new GRP_TXT value");
static_assert(PAYLOAD_TYPE_GRP_DATA == 0x06, "Review Public matching for the new GRP_DATA value");

// The default "Public" channel key (base64 izOH6cXN6mrJ5e26oRXNcg==), published
// in the MeshCore source and FAQ. Zero-padded to 32 bytes, as MeshCore keys
// the channel HMAC with a PUB_KEY_SIZE buffer.
const uint8_t PUBLIC_CHANNEL_SECRET[PUB_KEY_SIZE] = {
  0x8b, 0x33, 0x87, 0xe9, 0xc5, 0xcd, 0xea, 0x6a,
  0xc9, 0xe5, 0xed, 0xba, 0xa1, 0x15, 0xcd, 0x72,
};

// First byte of SHA-256 over the 16-byte key.
const uint8_t PUBLIC_CHANNEL_HASH = 0x11;

bool macMatches(const uint8_t* secret, const uint8_t* mac_and_data, int len) {
  if (len <= CIPHER_MAC_SIZE) return false;
  int enc_len = len - CIPHER_MAC_SIZE;
  if (enc_len % CIPHER_BLOCK_SIZE != 0) return false;

  // Same HMAC-SHA256 that Utils::MACThenDecrypt() checks, without the decrypt.
  uint8_t mac[CIPHER_MAC_SIZE];
  SHA256 sha;
  sha.resetHMAC(secret, PUB_KEY_SIZE);
  sha.update(mac_and_data + CIPHER_MAC_SIZE, enc_len);
  sha.finalizeHMAC(secret, PUB_KEY_SIZE, mac, CIPHER_MAC_SIZE);
  return memcmp(mac, mac_and_data, CIPHER_MAC_SIZE) == 0;
}

}

bool isPublicChannelPacket(const mesh::Packet* packet) {
  uint8_t type = packet->getPayloadType();
  if (type != PAYLOAD_TYPE_GRP_TXT && type != PAYLOAD_TYPE_GRP_DATA) return false;
  if (packet->payload_len < 1 + CIPHER_MAC_SIZE + CIPHER_BLOCK_SIZE) return false;
  if (packet->payload[0] != PUBLIC_CHANNEL_HASH) return false;  // cheap check first
  return macMatches(PUBLIC_CHANNEL_SECRET, &packet->payload[1], packet->payload_len - 1);
}
