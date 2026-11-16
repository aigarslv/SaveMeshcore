#pragma once

// SaveMeshcore: identifies group-channel packets by channel key.
//
// A group packet names its channel with one hash byte, which many channels
// share. The packet's 2-byte MAC is computed with the channel key, so a MAC
// that verifies proves the channel. Only the MAC is checked: nothing is
// decrypted, so the repeater cannot read the messages it matches.

#include <Packet.h>

// True if the packet is GRP_TXT or GRP_DATA on the default "Public" channel.
bool isPublicChannelPacket(const mesh::Packet* packet);

// Checks a fixed Public-channel vector through the same HMAC path the filter
// uses (CC310 hardware on nRF52, software elsewhere). Returns true on success.
bool publicChannelSelfTest();

// "cc310" or "software": which HMAC implementation this build uses.
const char* publicChannelHmacBackend();

