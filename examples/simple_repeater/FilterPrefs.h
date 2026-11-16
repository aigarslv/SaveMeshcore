#pragma once

// SaveMeshcore repeater filter settings.
//
// Stored in their own file instead of /prefs.json, so official MeshCore
// firmware never rewrites or drops them. Switching to stock firmware and back
// therefore keeps the operator's filter choices.

#include <Arduino.h>
#include <helpers/IdentityStore.h>   // FILESYSTEM
#include <helpers/ConfigSerializer.h>

#define FILTER_PREFS_FILE  "/savemesh_prefs"

class FilterPrefs : public ConfigSerializer {
public:
  uint8_t filter_1byte = 0;   // drop multi-hop flood packets with 1-byte path hashes
  uint8_t filter_data = 1;    // allow zero-hop GRP_DATA;
                              // drop relayed flood/direct GRP_DATA and 0x0C-0x0F
  uint8_t filter_public = 0;  // drop group messages on the default Public channel

  // Loads FILTER_PREFS_FILE. If it does not exist yet, imports the values an
  // earlier SaveMeshcore build kept in /prefs.json, then writes the new file.
  void begin(FILESYSTEM* fs);
  bool save(FILESYSTEM* fs);

protected:
  void structure() override {
    def("f_1byte", filter_1byte);
    def("f_data", filter_data);
    def("f_public", filter_public);
  }

private:
  bool loadFile(FILESYSTEM* fs, const char* filename, ConfigSerializer& target);
  void sanitise();
};
