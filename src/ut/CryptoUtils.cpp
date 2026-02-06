#include "CryptoUtils.hpp"

#ifdef UNDYING_TERMINAL_REQUIRE_DEPS
#include <sodium.h>

#include <vector>

bool CryptoUtils::DecodeHexKey(const std::string& hex, std::array<unsigned char, 32>* key) {
  if (!key) {
    return false;
  }
  if (hex.size() != 64) {
    return false;
  }
  size_t bin_len = 0;
  if (sodium_hex2bin(key->data(), key->size(), hex.c_str(), hex.size(), nullptr, &bin_len, nullptr) != 0) {
    return false;
  }
  return bin_len == key->size();
}

bool CryptoUtils::EncryptMessage(const std::string& message, const std::array<unsigned char, 32>& key, std::string* out) {
  if (!out) {
    return false;
  }
  std::vector<unsigned char> nonce(crypto_secretbox_NONCEBYTES);
  randombytes_buf(nonce.data(), nonce.size());

  std::vector<unsigned char> cipher(message.size() + crypto_secretbox_MACBYTES);
  if (crypto_secretbox_easy(cipher.data(), reinterpret_cast<const unsigned char*>(message.data()), message.size(), nonce.data(), key.data()) != 0) {
    return false;
  }

  out->assign(reinterpret_cast<const char*>(nonce.data()), nonce.size());
  out->append(reinterpret_cast<const char*>(cipher.data()), cipher.size());
  return true;
}

bool CryptoUtils::DecryptMessage(const std::string& payload, const std::array<unsigned char, 32>& key, std::string* out) {
  if (!out) {
    return false;
  }
  if (payload.size() < crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES) {
    return false;
  }

  const unsigned char* nonce = reinterpret_cast<const unsigned char*>(payload.data());
  const unsigned char* cipher = reinterpret_cast<const unsigned char*>(payload.data() + crypto_secretbox_NONCEBYTES);
  const size_t cipher_len = payload.size() - crypto_secretbox_NONCEBYTES;

  std::vector<unsigned char> message(cipher_len - crypto_secretbox_MACBYTES);
  if (crypto_secretbox_open_easy(message.data(), cipher, cipher_len, nonce, key.data()) != 0) {
    return false;
  }

  out->assign(reinterpret_cast<const char*>(message.data()), message.size());
  return true;
}
#else
bool CryptoUtils::DecodeHexKey(const std::string& hex, std::array<unsigned char, 32>* key) {
  (void)hex;
  (void)key;
  return false;
}

bool CryptoUtils::EncryptMessage(const std::string& message, const std::array<unsigned char, 32>& key, std::string* out) {
  (void)message;
  (void)key;
  (void)out;
  return false;
}

bool CryptoUtils::DecryptMessage(const std::string& payload, const std::array<unsigned char, 32>& key, std::string* out) {
  (void)payload;
  (void)key;
  (void)out;
  return false;
}
#endif
