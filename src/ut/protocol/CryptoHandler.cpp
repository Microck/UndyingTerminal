#include "CryptoHandler.hpp"

#include <cstring>
#include <stdexcept>

namespace ut {
CryptoHandler::CryptoHandler(const std::string& key, unsigned char nonce_msb) {
  std::lock_guard<std::mutex> guard(mutex_);
#ifdef UNDYING_TERMINAL_REQUIRE_DEPS
  if (sodium_init() < 0) {
    throw std::runtime_error("libsodium init failed");
  }
  if (key.size() != crypto_secretbox_KEYBYTES) {
    throw std::runtime_error("invalid key length");
  }
  std::memcpy(key_, key.data(), key.size());
  std::memset(nonce_, 0, sizeof(nonce_));
  nonce_[crypto_secretbox_NONCEBYTES - 1] = nonce_msb;
#else
  (void)key;
  (void)nonce_msb;
#endif
}

std::string CryptoHandler::Encrypt(const std::string& buffer) {
  std::lock_guard<std::mutex> guard(mutex_);
#ifdef UNDYING_TERMINAL_REQUIRE_DEPS
  IncrementNonce();
  std::string out(buffer.size() + crypto_secretbox_MACBYTES, '\0');
  if (crypto_secretbox_easy(reinterpret_cast<unsigned char*>(&out[0]),
                            reinterpret_cast<const unsigned char*>(buffer.data()),
                            buffer.size(), nonce_, key_) != 0) {
    throw std::runtime_error("encrypt failed");
  }
  return out;
#else
  return buffer;
#endif
}

std::string CryptoHandler::Decrypt(const std::string& buffer) {
  std::lock_guard<std::mutex> guard(mutex_);
#ifdef UNDYING_TERMINAL_REQUIRE_DEPS
  IncrementNonce();
  if (buffer.size() < crypto_secretbox_MACBYTES) {
    throw std::runtime_error("decrypt failed: short buffer");
  }
  std::string out(buffer.size() - crypto_secretbox_MACBYTES, '\0');
  if (crypto_secretbox_open_easy(reinterpret_cast<unsigned char*>(&out[0]),
                                 reinterpret_cast<const unsigned char*>(buffer.data()),
                                 buffer.size(), nonce_, key_) != 0) {
    throw std::runtime_error("decrypt failed");
  }
  return out;
#else
  return buffer;
#endif
}

void CryptoHandler::IncrementNonce() {
  for (size_t i = 0; i < sizeof(nonce_); ++i) {
    nonce_[i]++;
    if (nonce_[i] != 0) {
      break;
    }
  }
}
}
