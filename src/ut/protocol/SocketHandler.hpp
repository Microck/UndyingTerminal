#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>

#include "Packet.hpp"
#include "SocketTypes.hpp"

namespace ut {
class SocketHandler {
 public:
  virtual ~SocketHandler() = default;

  virtual bool HasData(SocketHandle socket) = 0;
  virtual int Read(SocketHandle socket, void* buf, size_t count) = 0;
  virtual int Write(SocketHandle socket, const void* buf, size_t count) = 0;
  virtual void Close(SocketHandle socket) = 0;

  void ReadAll(SocketHandle socket, void* buf, size_t count, bool timeout);
  void WriteAllOrThrow(SocketHandle socket, const void* buf, size_t count, bool timeout);

  template <typename T>
  T ReadProto(SocketHandle socket, bool timeout) {
    T t;
    int64_t length = 0;
    ReadAll(socket, &length, sizeof(int64_t), timeout);
    if (length < 0 || length > 128 * 1024 * 1024) {
      throw std::runtime_error("invalid proto length");
    }
    if (length == 0) {
      return t;
    }
    std::string s(static_cast<size_t>(length), '\0');
    ReadAll(socket, &s[0], static_cast<size_t>(length), timeout);
    if (!t.ParseFromString(s)) {
      throw std::runtime_error("invalid proto");
    }
    return t;
  }

  template <typename T>
  void WriteProto(SocketHandle socket, const T& t, bool timeout) {
    std::string s;
    if (!t.SerializeToString(&s)) {
      throw std::runtime_error("proto serialize failed");
    }
    int64_t length = static_cast<int64_t>(s.size());
    if (length < 0 || length > 128 * 1024 * 1024) {
      throw std::runtime_error("invalid proto length");
    }
    WriteAllOrThrow(socket, &length, sizeof(int64_t), timeout);
    if (length > 0) {
      WriteAllOrThrow(socket, s.data(), static_cast<size_t>(length), timeout);
    }
  }

  bool ReadPacket(SocketHandle socket, Packet* packet);
  void WritePacket(SocketHandle socket, const Packet& packet);
};
}
