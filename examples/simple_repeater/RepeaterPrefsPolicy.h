#pragma once

#include <stdint.h>

namespace savemesh {

constexpr uint8_t MIN_PATH_HASH_MODE = 1;       // 2-byte path hashes
constexpr uint8_t MAX_PATH_HASH_MODE = 2;       // 3-byte path hashes
constexpr uint8_t REQUIRED_MULTI_ACKS = 1;
constexpr uint8_t MIN_LOOP_DETECT = 1;           // minimal
constexpr uint8_t MAX_LOOP_DETECT = 3;           // strict
constexpr uint8_t REQUIRED_ADVERT_INTERVAL = 0;  // disable zero-hop adverts
constexpr uint8_t MIN_FLOOD_ADVERT_INTERVAL = 47;
constexpr uint8_t MAX_FLOOD_ADVERT_INTERVAL = 168;

inline bool isPathHashModeAllowed(uint16_t value) {
  return value >= MIN_PATH_HASH_MODE && value <= MAX_PATH_HASH_MODE;
}

inline bool isMultiAcksAllowed(uint16_t value) {
  return value == REQUIRED_MULTI_ACKS;
}

inline bool isAdvertIntervalAllowed(uint16_t value) {
  return value == REQUIRED_ADVERT_INTERVAL;
}

inline bool isFloodAdvertIntervalAllowed(uint16_t value) {
  return value >= MIN_FLOOD_ADVERT_INTERVAL && value <= MAX_FLOOD_ADVERT_INTERVAL;
}

template<typename Prefs>
bool repair(Prefs& prefs) {
  bool changed = false;

  if (prefs.path_hash_mode < MIN_PATH_HASH_MODE) {
    prefs.path_hash_mode = MIN_PATH_HASH_MODE;
    changed = true;
  } else if (prefs.path_hash_mode > MAX_PATH_HASH_MODE) {
    prefs.path_hash_mode = MAX_PATH_HASH_MODE;
    changed = true;
  }
  if (prefs.multi_acks != REQUIRED_MULTI_ACKS) {
    prefs.multi_acks = REQUIRED_MULTI_ACKS;
    changed = true;
  }
  if (prefs.loop_detect < MIN_LOOP_DETECT) {
    prefs.loop_detect = MIN_LOOP_DETECT;
    changed = true;
  } else if (prefs.loop_detect > MAX_LOOP_DETECT) {
    prefs.loop_detect = MAX_LOOP_DETECT;
    changed = true;
  }
  if (prefs.advert_interval != REQUIRED_ADVERT_INTERVAL) {
    prefs.advert_interval = REQUIRED_ADVERT_INTERVAL;
    changed = true;
  }
  if (prefs.flood_advert_interval < MIN_FLOOD_ADVERT_INTERVAL) {
    prefs.flood_advert_interval = MIN_FLOOD_ADVERT_INTERVAL;
    changed = true;
  } else if (prefs.flood_advert_interval > MAX_FLOOD_ADVERT_INTERVAL) {
    prefs.flood_advert_interval = MAX_FLOOD_ADVERT_INTERVAL;
    changed = true;
  }
  return changed;
}

}
