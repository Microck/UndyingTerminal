#pragma once

#include <string>

class ClientId {
 public:
  static std::string Generate();
  static std::string GetOrCreate();
  static std::string GetStored();
  static bool Store(const std::string& id);
  static bool Clear();

 private:
  static const char* kRegistryKey;
  static const char* kValueName;
};
