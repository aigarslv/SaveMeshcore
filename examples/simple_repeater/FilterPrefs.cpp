#include "FilterPrefs.h"

namespace {

// Reads the filter keys that earlier SaveMeshcore builds stored inside
// /prefs.json (repeat.f_1byte, repeat.f_data). Every other key is ignored.
class LegacyRepeatPrefs : public ConfigSerializer {
  FilterPrefs* _dest;
protected:
  void structure() override {
    def("f_1byte", _dest->filter_1byte);
    def("f_data", _dest->filter_data);
  }
public:
  LegacyRepeatPrefs(FilterPrefs* dest) : _dest(dest) { }
};

class LegacyNodePrefs : public ConfigSerializer {
  LegacyRepeatPrefs _repeat;
protected:
  void structure() override {
    def("repeat", _repeat);
  }
public:
  LegacyNodePrefs(FilterPrefs* dest) : _repeat(dest) { }
};

}

bool FilterPrefs::loadFile(FILESYSTEM* fs, const char* filename, ConfigSerializer& target) {
  if (!fs->exists(filename)) return false;
#if defined(RP2040_PLATFORM)
  auto file = fs->open(filename, "r");
#else
  auto file = fs->open(filename);
#endif
  if (!file) return false;
  bool ok = target.loadSerial(file);
  file.close();
  return ok;
}

void FilterPrefs::sanitise() {
  filter_1byte = filter_1byte ? 1 : 0;
  filter_data = filter_data ? 1 : 0;
}

void FilterPrefs::begin(FILESYSTEM* fs) {
  if (loadFile(fs, FILTER_PREFS_FILE, *this)) {
    sanitise();
    return;
  }

  // First boot with this build: keep any choice saved by an older build.
  // Keys missing from /prefs.json keep the defaults declared in the header.
  LegacyNodePrefs legacy(this);
  loadFile(fs, "/prefs.json", legacy);
  sanitise();
  save(fs);
}

bool FilterPrefs::save(FILESYSTEM* fs) {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  fs->remove(FILTER_PREFS_FILE);
  auto file = fs->open(FILTER_PREFS_FILE, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  auto file = fs->open(FILTER_PREFS_FILE, "w");
#else
  auto file = fs->open(FILTER_PREFS_FILE, "w", true);
#endif
  if (!file) return false;
  bool ok = saveSerial(file);
  file.close();
  return ok;
}
