#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
  return value == 0 || (value >= MIN_FLOOD_ADVERT_INTERVAL && value <= MAX_FLOOD_ADVERT_INTERVAL);
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
  if (prefs.flood_advert_interval > 0
      && prefs.flood_advert_interval < MIN_FLOOD_ADVERT_INTERVAL) {
    prefs.flood_advert_interval = MIN_FLOOD_ADVERT_INTERVAL;
    changed = true;
  } else if (prefs.flood_advert_interval > MAX_FLOOD_ADVERT_INTERVAL) {
    prefs.flood_advert_interval = MAX_FLOOD_ADVERT_INTERVAL;
    changed = true;
  }
  return changed;
}

// ---------------------------------------------------------------------------
// Command validation
//
// Protected "set ..." commands are validated here and then forwarded to the
// stock CLI as a *canonical* string rebuilt from the validated value. Stock
// parsers are not uniform (some skip extra spaces, some read 0 instead), so
// forwarding the user's raw text could store something other than what was
// validated.
// ---------------------------------------------------------------------------

inline bool isBlankChar(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

inline bool isBlankText(const char* text) {
  while (*text) {
    if (!isBlankChar(*text++)) return false;
  }
  return true;
}

// Matches "<name>" optionally followed by spaces and an argument. Sets *arg to
// the first non-space character after the name.
inline bool matchCommand(const char* command, const char* name, const char** arg) {
  size_t len = strlen(name);
  if (strncmp(command, name, len) != 0 || (command[len] != 0 && command[len] != ' ')) return false;
  *arg = command + len;
  while (**arg == ' ') (*arg)++;
  return true;
}

// Strict unsigned decimal: digits only (optionally followed by whitespace).
inline bool parseUnsigned(const char* arg, uint16_t* value) {
  if (*arg < '0' || *arg > '9') return false;
  uint32_t result = 0;
  while (*arg >= '0' && *arg <= '9') {
    result = result * 10 + (uint32_t)(*arg++ - '0');
    if (result > UINT16_MAX) return false;
  }
  if (!isBlankText(arg)) return false;
  *value = (uint16_t)result;
  return true;
}

inline bool matchesWord(const char* arg, const char* word) {
  size_t len = strlen(word);
  return strncmp(arg, word, len) == 0 && isBlankText(arg + len);
}

enum class ProtectedKey : uint8_t {
  NONE, PATH_HASH_MODE, MULTI_ACKS, LOOP_DETECT, ADVERT_INTERVAL, FLOOD_ADVERT_INTERVAL
};

struct ProtectedCheck {
  bool matched = false;                 // command names a protected setting
  bool allowed = false;                 // value passes policy (canonical is valid)
  ProtectedKey key = ProtectedKey::NONE;
  const char* error = "";               // reply text when matched && !allowed
};

// Validates a protected "set" command. When allowed, writes the canonical
// command to `canonical`; forward that string, never the original.
inline ProtectedCheck checkProtectedSet(const char* command, char* canonical, size_t capacity) {
  ProtectedCheck r;
  const char* arg;
  uint16_t value;

  if (matchCommand(command, "set path.hash.mode", &arg)) {
    r.matched = true; r.key = ProtectedKey::PATH_HASH_MODE;
    if (parseUnsigned(arg, &value) && isPathHashModeAllowed(value)) {
      r.allowed = true;
      snprintf(canonical, capacity, "set path.hash.mode %u", (unsigned)value);
    } else {
      r.error = "Error: path.hash.mode must be 1 or 2";
    }
  } else if (matchCommand(command, "set multi.acks", &arg)) {
    r.matched = true; r.key = ProtectedKey::MULTI_ACKS;
    if (parseUnsigned(arg, &value) && isMultiAcksAllowed(value)) {
      r.allowed = true;
      snprintf(canonical, capacity, "set multi.acks %u", (unsigned)value);
    } else {
      r.error = "Error: multi.acks must be 1";
    }
  } else if (matchCommand(command, "set loop.detect", &arg)) {
    r.matched = true; r.key = ProtectedKey::LOOP_DETECT;
    const char* word = matchesWord(arg, "minimal") ? "minimal"
                     : matchesWord(arg, "moderate") ? "moderate"
                     : matchesWord(arg, "strict") ? "strict" : nullptr;
    if (word) {
      r.allowed = true;
      snprintf(canonical, capacity, "set loop.detect %s", word);
    } else {
      r.error = "Error: loop.detect must be minimal, moderate or strict";
    }
  } else if (matchCommand(command, "set advert.interval", &arg)) {
    r.matched = true; r.key = ProtectedKey::ADVERT_INTERVAL;
    if (parseUnsigned(arg, &value) && isAdvertIntervalAllowed(value)) {
      r.allowed = true;
      snprintf(canonical, capacity, "set advert.interval %u", (unsigned)value);
    } else {
      r.error = "Error: advert.interval must be 0";
    }
  } else if (matchCommand(command, "set flood.advert.interval", &arg)) {
    r.matched = true; r.key = ProtectedKey::FLOOD_ADVERT_INTERVAL;
    if (parseUnsigned(arg, &value) && isFloodAdvertIntervalAllowed(value)) {
      r.allowed = true;
      snprintf(canonical, capacity, "set flood.advert.interval %u", (unsigned)value);
    } else {
      r.error = "Error: flood.advert.interval must be 0 or 47-168 hours";
    }
  }
  return r;
}

}
