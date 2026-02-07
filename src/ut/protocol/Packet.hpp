#pragma once

#include <cstdint>
#include <string>

namespace ut { 
class Packet {
 public:
  Packet() : encrypted_(false), header_(255) {}
  Packet(uint8_t header, const std::string& payload)
      : encrypted_(false), header_(header), payload_(payload) {}
  Packet(bool encrypted, uint8_t header, const std::string& payload)
      : encrypted_(encrypted), header_(header), payload_(payload) {}
  explicit Packet(const std::string& serialized) {
    if (serialized.size() < 2) {
      encrypted_ = false;
      header_ = 255;
      return;
    }
    encrypted_ = serialized[0] != 0;
    header_ = static_cast<uint8_t>(serialized[1]);
    payload_ = serialized.substr(2);
  }

  bool is_encrypted() const { return encrypted_; }
  uint8_t header() const { return header_; }
  const std::string& payload() const { return payload_; }

  void set_encrypted(bool encrypted) { encrypted_ = encrypted; }
  void set_header(uint8_t header) { header_ = header; }
  void set_payload(const std::string& payload) { payload_ = payload; }

  std::string serialize() const {
    std::string out("00", 2);
    out[0] = encrypted_ ? 1 : 0;
    out[1] = static_cast<char>(header_);
    out.append(payload_);
    return out;
  }

  size_t length() const { return 2 + payload_.size(); }

 private:
  bool encrypted_;
  uint8_t header_;
  std::string payload_;
};
}
