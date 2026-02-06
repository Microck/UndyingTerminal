#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "CryptoHandler.hpp"
#include "Packet.hpp"
#include "SocketHandler.hpp"

namespace ut {
class BackedReader {
 public:
  BackedReader(std::shared_ptr<SocketHandler> socket_handler,
               std::shared_ptr<CryptoHandler> crypto_handler,
               SocketHandle socket);

  bool HasData();
  int Read(Packet* packet);
  void Revive(SocketHandle socket, const std::vector<std::string>& buffered);
  void InvalidateSocket();
  int64_t sequence_number() const { return sequence_number_; }
  std::mutex& recover_mutex() { return recover_mutex_; }

 private:
  int GetPartialMessageLength() const;
  void ConstructPartialMessage(Packet* packet);

  std::mutex recover_mutex_;
  std::shared_ptr<SocketHandler> socket_handler_;
  std::shared_ptr<CryptoHandler> crypto_handler_;
  SocketHandle socket_;
  int64_t sequence_number_ = 0;
  std::deque<std::string> local_buffer_;
  std::string partial_message_;
};
}
