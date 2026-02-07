#include "BackedReader.hpp"

#include <cstring>
#include <stdexcept>

#include <winsock2.h>

namespace ut {
BackedReader::BackedReader(std::shared_ptr<SocketHandler> socket_handler,
                           std::shared_ptr<CryptoHandler> crypto_handler,
                           SocketHandle socket)
    : socket_handler_(std::move(socket_handler)),
      crypto_handler_(std::move(crypto_handler)),
      socket_(socket) {}

bool BackedReader::HasData() {
  std::lock_guard<std::mutex> guard(recover_mutex_);
  if (socket_ == kInvalidSocket) {
    return false;
  }
  if (!local_buffer_.empty()) {
    return true;
  }
  return socket_handler_->HasData(socket_);
}

int BackedReader::Read(Packet* packet) {
  std::lock_guard<std::mutex> guard(recover_mutex_);
  if (socket_ == kInvalidSocket) {
    return 0;
  }
  if (!local_buffer_.empty()) {
    *packet = Packet(local_buffer_.front());
    local_buffer_.pop_front();
    if (packet->is_encrypted()) {
      packet->set_payload(crypto_handler_->Decrypt(packet->payload()));
      packet->set_encrypted(false);
    }
    return 1;
  }

  if (partial_message_.size() < 4) {
    char tmp[4] = {};
    const int rc = socket_handler_->Read(socket_, tmp, 4 - partial_message_.size());
    if (rc == 0) {
      return -1;
    }
    if (rc < 0) {
      return -1;
    }
    partial_message_.append(tmp, tmp + rc);
  }
  if (partial_message_.size() < 4) {
    return 0;
  }

  const int message_length = GetPartialMessageLength();
  const int remaining = message_length - static_cast<int>(partial_message_.size() - 4);
  if (remaining > 0) {
    std::string s(static_cast<size_t>(remaining), '\0');
    const int rc = socket_handler_->Read(socket_, &s[0], s.size());
    if (rc == 0) {
      return -1;
    }
    if (rc < 0) {
      return -1;
    }
    partial_message_.append(s.data(), s.data() + rc);
  }
  if (static_cast<int>(partial_message_.size() - 4) == message_length) {
    ConstructPartialMessage(packet);
    return 1;
  }
  return 0;
}

void BackedReader::Revive(SocketHandle socket, const std::vector<std::string>& buffered) {
  partial_message_.clear();
  local_buffer_.insert(local_buffer_.end(), buffered.begin(), buffered.end());
  sequence_number_ += static_cast<int64_t>(buffered.size());
  socket_ = socket;
}

void BackedReader::InvalidateSocket() {
  std::lock_guard<std::mutex> guard(recover_mutex_);
  socket_ = kInvalidSocket;
}

int BackedReader::GetPartialMessageLength() const {
  if (partial_message_.size() < 4) {
    throw std::runtime_error("partial header missing");
  }
  uint32_t len_be = 0;
  std::memcpy(&len_be, partial_message_.data(), sizeof(len_be));
  const uint32_t len = ntohl(len_be);
  if (len == 0 || len > 128 * 1024 * 1024) {
    throw std::runtime_error("invalid message length");
  }
  return static_cast<int>(len);
}

void BackedReader::ConstructPartialMessage(Packet* packet) {
  const int message_length = GetPartialMessageLength();
  const size_t body_offset = 4;
  if (partial_message_.size() - body_offset != static_cast<size_t>(message_length)) {
    throw std::runtime_error("partial message length mismatch");
  }
  const std::string serialized = partial_message_.substr(body_offset, message_length);
  *packet = Packet(serialized);
  if (packet->is_encrypted()) {
    packet->set_payload(crypto_handler_->Decrypt(packet->payload()));
    packet->set_encrypted(false);
  }
  partial_message_.clear();
  sequence_number_++;
}
}
