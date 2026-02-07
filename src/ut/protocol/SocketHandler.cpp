#include "SocketHandler.hpp"

#include <chrono>
#include <thread>

#include <winsock2.h>

namespace ut {
namespace {
constexpr int kSocketTimeoutSeconds = 30;
}

void SocketHandler::ReadAll(SocketHandle socket, void* buf, size_t count, bool timeout) {
  size_t pos = 0;
  auto start = std::chrono::steady_clock::now();
  while (pos < count) {
    if (!HasData(socket)) {
      if (timeout) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if (elapsed > kSocketTimeoutSeconds) {
          throw std::runtime_error("socket timeout");
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }

    const int rc = Read(socket, static_cast<char*>(buf) + pos, count - pos);
    if (rc == 0) {
      throw std::runtime_error("socket closed");
    }
    if (rc < 0) {
      throw std::runtime_error("socket read failed");
    }
    pos += static_cast<size_t>(rc);
    start = std::chrono::steady_clock::now();
  }
}

void SocketHandler::WriteAllOrThrow(SocketHandle socket, const void* buf, size_t count, bool timeout) {
  size_t pos = 0;
  auto start = std::chrono::steady_clock::now();
  while (pos < count) {
    if (timeout) {
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
      if (elapsed > kSocketTimeoutSeconds) {
        throw std::runtime_error("socket timeout");
      }
    }
    const int rc = Write(socket, static_cast<const char*>(buf) + pos, count - pos);
    if (rc == 0) {
      throw std::runtime_error("socket closed during write");
    }
    if (rc < 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }
    pos += static_cast<size_t>(rc);
    start = std::chrono::steady_clock::now();
  }
}

bool SocketHandler::ReadPacket(SocketHandle socket, Packet* packet) {
  if (!packet) {
    return false;
  }
  uint32_t length_be = 0;
  ReadAll(socket, &length_be, sizeof(uint32_t), false);
  const uint32_t length = ntohl(length_be);
  if (length == 0 || length > 128 * 1024 * 1024) {
    throw std::runtime_error("invalid packet length");
  }
  std::string s(length, '\0');
  ReadAll(socket, &s[0], length, false);
  *packet = Packet(s);
  return true;
}

void SocketHandler::WritePacket(SocketHandle socket, const Packet& packet) {
  std::string data = packet.serialize();
  if (data.size() > 128 * 1024 * 1024) {
    throw std::runtime_error("invalid packet length");
  }
  uint32_t length_be = htonl(static_cast<uint32_t>(data.size()));
  WriteAllOrThrow(socket, &length_be, sizeof(uint32_t), false);
  WriteAllOrThrow(socket, data.data(), data.size(), false);
}
}
