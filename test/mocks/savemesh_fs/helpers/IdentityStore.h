#pragma once

// In-memory filesystem for the SaveMeshcore preference tests. Only the
// native_savemesh_prefs environment puts this directory on its include path, so
// it replaces helpers/IdentityStore.h (and its FILESYSTEM type) for that build alone.

#include <Arduino.h>
#include <map>
#include <string>

struct MockFile : public Stream {
  std::string* data = nullptr;
  size_t pos = 0;
  bool* fail_writes = nullptr;

  explicit operator bool() const { return data != nullptr; }
  int available() override { return data ? (int)(data->size() - pos) : 0; }
  int read() override { return (data && pos < data->size()) ? (unsigned char)(*data)[pos++] : -1; }
  int peek() override { return (data && pos < data->size()) ? (unsigned char)(*data)[pos] : -1; }
  size_t write(uint8_t b) override {
    if (!data || (fail_writes && *fail_writes)) return 0;
    data->push_back((char)b);
    return 1;
  }
  size_t print(unsigned char v, int r = DEC) override { return Print::print(std::to_string(v).c_str()); }
  size_t print(int v, int r = DEC) override { return Print::print(std::to_string(v).c_str()); }
  size_t print(unsigned int v, int r = DEC) override { return Print::print(std::to_string(v).c_str()); }
  size_t print(long v, int r = DEC) override { return Print::print(std::to_string(v).c_str()); }
  size_t print(unsigned long v, int r = DEC) override { return Print::print(std::to_string(v).c_str()); }
  using Print::print;
  void close() {}
};

struct MockFS {
  std::map<std::string, std::string> files;
  bool fail_writes = false;     // simulate a full or failing filesystem

  bool exists(const char* name) { return files.count(name) > 0; }
  MockFile open(const char* name) {
    MockFile f;
    auto it = files.find(name);
    if (it != files.end()) f.data = &it->second;
    return f;
  }
  MockFile open(const char* name, const char*, bool) {
    MockFile f;
    files[name].clear();
    f.data = &files[name];
    f.fail_writes = &fail_writes;
    return f;
  }
  bool remove(const char* name) { return files.erase(name) > 0; }
};

#define FILESYSTEM MockFS
