#include "BackedWriter.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <winsock2.h>

namespace ut {
namespace {
bool DebugHandshake() {
  return std::getenv("UT_DEBUG_HANDSHAKE") != nullptr;
}
}
BackedWriter::BackedWriter(std::shared_ptr<SocketHandler> socket_handler,
                           std::shared_ptr<CryptoHandler> crypto_handler,
                           SocketHandle socket)
    : socket_handler_(std::move(socket_handler)),
      crypto_handler_(std::move(crypto_handler)),
      socket_(socket) {}

BackedWriterWriteState BackedWriter::Write(Packet packet) {
  {
    std::lock_guard<std::mutex> guard(recover_mutex_);
    if (socket_ == kInvalidSocket) {
      if (DebugHandshake()) {
        std::cerr << "[handshake] writer socket invalid\n";
      }
      return BackedWriterWriteState::Skipped;
    }

    packet.set_encrypted(true);
    packet.set_payload(crypto_handler_->Encrypt(packet.payload()));

    backup_buffer_.push_front(packet);
    backup_size_ += static_cast<int64_t>(packet.length());
    sequence_number_++;

    while (backup_size_ > et::kMaxBackupBytes && !backup_buffer_.empty()) {
      backup_size_ -= static_cast<int64_t>(backup_buffer_.back().length());
      backup_buffer_.pop_back();
    }
  }

  std::string data = packet.serialize();
  const uint32_t total_len = static_cast<uint32_t>(data.size());
  uint32_t len_be = htonl(total_len);

  size_t bytes_written = 0;
  std::string framed(reinterpret_cast<const char*>(&len_be), sizeof(len_be));
  framed.append(data);


  while (true) {
    if (socket_ == kInvalidSocket) {
      return BackedWriterWriteState::WroteWithFailure;
    }
    int rc = socket_handler_->Write(socket_, framed.data() + bytes_written, framed.size() - bytes_written);
    if (rc >= 0) {
      bytes_written += static_cast<size_t>(rc);
      if (bytes_written == framed.size()) {
        return BackedWriterWriteState::Success;
      }
    } else {
      if (DebugHandshake()) {
        std::cerr << "[handshake] writer write failed\n";
      }
      return BackedWriterWriteState::WroteWithFailure;
    }
  }
}

std::vector<std::string> BackedWriter::Recover(int64_t last_valid_sequence_number) {
  if (socket_ != kInvalidSocket) {
    throw std::runtime_error("recover with active socket");
  }
  int64_t messages_to_recover = sequence_number_ - last_valid_sequence_number;
  if (messages_to_recover < 0) {
    throw std::runtime_error("peer ahead of writer");
  }
  if (messages_to_recover == 0) {
    return {};
  }
  int64_t messages_seen = 0;
  std::vector<std::string> out;
  for (const auto& packet : backup_buffer_) {
    out.push_back(packet.serialize());
    messages_seen++;
    if (messages_seen == messages_to_recover) {
      std::reverse(out.begin(), out.end());
      return out;
    }
  }
  throw std::runtime_error("client too far behind server");
}

void BackedWriter::Revive(SocketHandle socket) {
  socket_ = socket;
}

void BackedWriter::InvalidateSocket() {
  std::lock_guard<std::mutex> guard(recover_mutex_);
  socket_ = kInvalidSocket;
}
}
