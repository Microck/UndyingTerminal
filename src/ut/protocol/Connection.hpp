#pragma once

#include <memory>
#include <mutex>

#include "BackedReader.hpp"
#include "BackedWriter.hpp"
#include "SocketHandler.hpp"

namespace ut {
class Connection {
 public:
  Connection(std::shared_ptr<SocketHandler> socket_handler,
             const std::string& id,
             const std::string& key);
  virtual ~Connection();

  bool ReadPacket(Packet* packet);
  void WritePacket(const Packet& packet);
  bool Read(Packet* packet);
  bool Write(const Packet& packet);

  void CloseSocket();
  virtual void CloseSocketAndMaybeReconnect() { CloseSocket(); }

  bool Recover(SocketHandle new_socket);
  void Shutdown();

  std::shared_ptr<BackedReader> reader() { return reader_; }
  std::shared_ptr<BackedWriter> writer() { return writer_; }
  SocketHandle socket() const { return socket_; }
  const std::string& id() const { return id_; }

 protected:
  std::shared_ptr<SocketHandler> socket_handler_;
  std::string id_;
  std::string key_;
  std::shared_ptr<BackedReader> reader_;
  std::shared_ptr<BackedWriter> writer_;
  SocketHandle socket_;
  bool shutting_down_ = false;
  std::recursive_mutex mutex_;
};
}
