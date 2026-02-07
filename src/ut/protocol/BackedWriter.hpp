#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "CryptoHandler.hpp"
#include "UtConstants.hpp"
#include "Packet.hpp"
#include "SocketHandler.hpp"

namespace ut {
enum class BackedWriterWriteState {
  Skipped = 0,
  Success = 1,
  WroteWithFailure = 2,
};

class BackedWriter {
 public:
  BackedWriter(std::shared_ptr<SocketHandler> socket_handler,
               std::shared_ptr<CryptoHandler> crypto_handler,
               SocketHandle socket);

  BackedWriterWriteState Write(Packet packet);
  std::vector<std::string> Recover(int64_t last_valid_sequence_number);
  void Revive(SocketHandle socket);
  void InvalidateSocket();

  std::mutex& recover_mutex() { return recover_mutex_; }
  int64_t sequence_number() const { return sequence_number_; }

 private:
  std::mutex recover_mutex_;
  std::shared_ptr<SocketHandler> socket_handler_;
  std::shared_ptr<CryptoHandler> crypto_handler_;
  SocketHandle socket_;
  std::deque<Packet> backup_buffer_;
  int64_t backup_size_ = 0;
  int64_t sequence_number_ = 0;
};
}
