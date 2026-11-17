#pragma once

// SaveMeshcore CLI parsing helpers. Header-only so the native tests can cover
// them directly.

#include <stdint.h>
#include <string.h>

namespace savemesh {

// True if text contains nothing but spaces (same rule as StrHelper::isBlank).
inline bool isSpacesOnly(const char* text) {
  while (*text) {
    if (*text++ != ' ') return false;
  }
  return true;
}

// Matches "<name>" either alone or followed by a space. Sets *arg to the first
// non-space character after the name.
inline bool matchCommand(const char* command, const char* name, const char** arg) {
  size_t len = strlen(name);
  if (strncmp(command, name, len) != 0 || (command[len] != 0 && command[len] != ' ')) return false;
  *arg = command + len;
  while (**arg == ' ') (*arg)++;
  return true;
}

// Strict unsigned decimal: digits only, optionally followed by spaces.
inline bool parseUnsigned(const char* arg, uint16_t* value) {
  if (*arg < '0' || *arg > '9') return false;
  uint32_t result = 0;
  while (*arg >= '0' && *arg <= '9') {
    result = result * 10 + (uint32_t)(*arg++ - '0');
    if (result > UINT16_MAX) return false;
  }
  if (!isSpacesOnly(arg)) return false;
  *value = (uint16_t)result;
  return true;
}

// Exact word, optionally followed by spaces.
inline bool matchesValue(const char* arg, const char* value) {
  size_t len = strlen(value);
  return strncmp(arg, value, len) == 0 && isSpacesOnly(arg + len);
}

inline bool isTrailingBlank(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

inline char lowerAscii(char c) {
  return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

// Parses "on" or "off" for the filter commands. Case-insensitive; trailing
// spaces, tabs and line endings are ignored. Returns false for anything else.
inline bool parseOnOff(const char* arg, uint8_t* value) {
  const char* end = arg + strlen(arg);
  while (end > arg && isTrailingBlank(end[-1])) end--;
  size_t len = (size_t)(end - arg);

  char word[4] = {0};
  if (len == 0 || len >= sizeof(word)) return false;
  for (size_t i = 0; i < len; i++) word[i] = lowerAscii(arg[i]);

  if (strcmp(word, "on") == 0) { *value = 1; return true; }
  if (strcmp(word, "off") == 0) { *value = 0; return true; }
  return false;
}

}
