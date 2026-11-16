#include <gtest/gtest.h>

#include "FilterPrefs.h"

// The filter settings live in /savemesh_prefs, separate from MeshCore's own
// /prefs.json. These tests cover first boot, migration from older SaveMeshcore
// builds, coexistence with stock firmware, and damaged files.

static std::string& sidecar(MockFS& fs) { return fs.files[FILTER_PREFS_FILE]; }

TEST(FilterPrefs, FreshDeviceGetsDefaultsAndWritesFile) {
  MockFS fs;
  FilterPrefs p;
  p.begin(&fs);
  EXPECT_EQ(p.filter_1byte, 0);
  EXPECT_EQ(p.filter_data, 1);
  EXPECT_EQ(p.filter_public, 0);
  EXPECT_EQ(sidecar(fs), "{f_1byte:0,f_data:1,f_public:0}");
}

TEST(FilterPrefs, StockPrefsFileWithoutFilterKeysKeepsDefaults) {
  MockFS fs;
  fs.files["/prefs.json"] = "{name:\"x\",repeat:{disable:0,f_max:64,loop:0},radio:{freq:869.5}}";
  FilterPrefs p;
  p.begin(&fs);
  EXPECT_EQ(p.filter_1byte, 0);
  EXPECT_EQ(p.filter_data, 1);
  EXPECT_EQ(p.filter_public, 0);
}

TEST(FilterPrefs, MigratesSettingsSavedByOlderBuilds) {
  MockFS fs;
  fs.files["/prefs.json"] = "{name:\"x\",repeat:{disable:0,loop:0,f_data:0,f_1byte:1},room:{rd_only:1}}";
  FilterPrefs p;
  p.begin(&fs);
  EXPECT_EQ(p.filter_data, 0);
  EXPECT_EQ(p.filter_1byte, 1);
  EXPECT_EQ(p.filter_public, 0);   // older builds had no public filter

  FilterPrefs again;               // second boot reads the new file
  again.begin(&fs);
  EXPECT_EQ(again.filter_data, 0);
  EXPECT_EQ(again.filter_1byte, 1);
}

TEST(FilterPrefs, SidecarWinsOverRewrittenStockPrefs) {
  // Stock firmware rewrote /prefs.json without our keys. The sidecar still holds the choice.
  MockFS fs;
  fs.files[FILTER_PREFS_FILE] = "{f_1byte:0,f_data:0,f_public:1}";
  fs.files["/prefs.json"] = "{repeat:{loop:0}}";
  FilterPrefs p;
  p.begin(&fs);
  EXPECT_EQ(p.filter_data, 0);
  EXPECT_EQ(p.filter_public, 1);
}

TEST(FilterPrefs, SaveAndReloadRoundTrip) {
  MockFS fs;
  FilterPrefs p;
  p.begin(&fs);
  p.filter_1byte = 1; p.filter_data = 0; p.filter_public = 1;
  ASSERT_TRUE(p.save(&fs));
  EXPECT_EQ(sidecar(fs), "{f_1byte:1,f_data:0,f_public:1}");

  FilterPrefs q;
  q.begin(&fs);
  EXPECT_EQ(q.filter_1byte, 1);
  EXPECT_EQ(q.filter_data, 0);
  EXPECT_EQ(q.filter_public, 1);
}

TEST(FilterPrefs, MissingKeyKeepsItsOwnDefault) {
  MockFS fs;
  fs.files[FILTER_PREFS_FILE] = "{f_1byte:1}";
  FilterPrefs p;
  p.begin(&fs);
  EXPECT_EQ(p.filter_1byte, 1);
  EXPECT_EQ(p.filter_data, 1);
  EXPECT_EQ(p.filter_public, 0);
}

TEST(FilterPrefs, NonBooleanValuesAreNormalised) {
  MockFS fs;
  fs.files[FILTER_PREFS_FILE] = "{f_1byte:7,f_data:0,f_public:200}";
  FilterPrefs p;
  p.begin(&fs);
  EXPECT_EQ(p.filter_1byte, 1);
  EXPECT_EQ(p.filter_data, 0);
  EXPECT_EQ(p.filter_public, 1);
}

TEST(FilterPrefs, DamagedFilesNeverCrashAndStayBoolean) {
  for (const char* contents : {"", "{}", "garbage", "{f_1byte:1,f_da", "{{{{{{{{{{{{{{{{{{{{",
                               "{f_1byte:-5,f_data:abc}", "}}}}", "{f_1byte:1,f_data:0,f_public:1,}"}) {
    MockFS fs;
    fs.files[FILTER_PREFS_FILE] = contents;
    FilterPrefs p;
    p.begin(&fs);
    EXPECT_LE(p.filter_1byte, 1) << contents;
    EXPECT_LE(p.filter_data, 1) << contents;
    EXPECT_LE(p.filter_public, 1) << contents;
  }
}

TEST(FilterPrefs, ReportsFailedSave) {
  MockFS fs;
  FilterPrefs p;
  p.begin(&fs);
  fs.fail_writes = true;
  p.filter_data = 0;
  EXPECT_FALSE(p.save(&fs));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
