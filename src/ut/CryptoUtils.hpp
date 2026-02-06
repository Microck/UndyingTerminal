#pragma once

#include <array>
#include <string>

class CryptoUtils {
 public:
  static bool DecodeHexKey(const std::string& hex, std::array<unsigned char, 32>* key);
  static bool EncryptMessage(const std::string& message, const std::array<unsigned char, 32>& key, std::string* out);
  static bool DecryptMessage(const std::string& payload, const std::array<unsigned char, 32>& key, std::string* out);
};
