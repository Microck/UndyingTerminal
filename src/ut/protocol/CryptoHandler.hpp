#pragma once

#include <mutex>
#include <string>

#ifdef UNDYING_TERMINAL_REQUIRE_DEPS
#include <sodium.h>
#endif

namespace ut {
class CryptoHandler {
 public:
  CryptoHandler(const std::string& key, unsigned char nonce_msb);

  std::string Encrypt(const std::string& buffer);
  std::string Decrypt(const std::string& buffer);

 private:
  void IncrementNonce();

  std::mutex mutex_;
  unsigned char nonce_[24] = {};
  unsigned char key_[32] = {};
};
}
