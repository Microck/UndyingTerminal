#include "Connection.hpp"

#include <chrono>
#include <thread>

#include "ET.pb.h"

namespace ut {
Connection::Connection(std::shared_ptr<SocketHandler> socket_handler,
                       const std::string& id,
                       const std::string& key)
    : socket_handler_(std::move(socket_handler)), id_(id), key_(key), socket_(kInvalidSocket) {}

Connection::~Connection() {
  if (!shutting_down_) {
    Shutdown();
  }
}

bool Connection::ReadPacket(Packet* packet) {
  return Read(packet);
}

void Connection::WritePacket(const Packet& packet) {
  while (true) {
    {
      std::lock_guard<std::recursive_mutex> guard(mutex_);
      if (shutting_down_) {
        return;
      }
    }
    if (Write(packet)) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
}

bool Connection::Read(Packet* packet) {
  std::shared_ptr<BackedReader> reader;
  {
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    if (shutting_down_) {
      return false;
    }
    reader = reader_;
  }
  if (!reader) {
    return false;
  }
  int rc = reader->Read(packet);
  if (rc == -1) {
    CloseSocketAndMaybeReconnect();
    return false;
  }
  return rc > 0;
}

bool Connection::Write(const Packet& packet) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (socket_ == kInvalidSocket || !writer_) {
    return false;
  }
  auto state = writer_->Write(packet);
  if (state == BackedWriterWriteState::Skipped) {
    return false;
  }
  if (state == BackedWriterWriteState::WroteWithFailure) {
    CloseSocketAndMaybeReconnect();
  }
  return true;
}

void Connection::CloseSocket() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  if (socket_ == kInvalidSocket) {
    return;
  }
  if (reader_) {
    reader_->InvalidateSocket();
  }
  if (writer_) {
    writer_->InvalidateSocket();
  }
  SocketHandle socket = socket_;
  socket_ = kInvalidSocket;
  socket_handler_->Close(socket);
}

bool Connection::Recover(SocketHandle new_socket) {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  std::lock_guard<std::mutex> reader_guard(reader_->recover_mutex());
  std::lock_guard<std::mutex> writer_guard(writer_->recover_mutex());
  try {
    et::SequenceHeader header;
    header.set_sequencenumber(static_cast<int32_t>(reader_->sequence_number()));
    socket_handler_->WriteProto(new_socket, header, true);

    et::SequenceHeader remote = socket_handler_->ReadProto<et::SequenceHeader>(new_socket, true);

    et::CatchupBuffer catchup;
    std::vector<std::string> recovered = writer_->Recover(remote.sequencenumber());
    for (const auto& entry : recovered) {
      catchup.add_buffer(entry);
    }
    socket_handler_->WriteProto(new_socket, catchup, true);

    et::CatchupBuffer inbound = socket_handler_->ReadProto<et::CatchupBuffer>(new_socket, true);
    std::vector<std::string> inbound_msgs(inbound.buffer().begin(), inbound.buffer().end());

    socket_ = new_socket;
    reader_->Revive(socket_, inbound_msgs);
    writer_->Revive(socket_);
    return true;
  } catch (...) {
    socket_handler_->Close(new_socket);
    return false;
  }
}

void Connection::Shutdown() {
  std::lock_guard<std::recursive_mutex> guard(mutex_);
  shutting_down_ = true;
  CloseSocket();
}
}
