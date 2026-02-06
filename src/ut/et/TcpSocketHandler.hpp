#pragma once

#include <string>

#include "SocketHandler.hpp"

namespace ut {
class TcpSocketHandler : public SocketHandler {
 public:
  TcpSocketHandler();
  ~TcpSocketHandler() override;

  bool HasData(SocketHandle socket) override;
  int Read(SocketHandle socket, void* buf, size_t count) override;
  int Write(SocketHandle socket, const void* buf, size_t count) override;
  void Close(SocketHandle socket) override;

  SocketHandle Connect(const std::string& host, int port);
  SocketHandle Listen(const std::string& bind_ip, int port);
  SocketHandle Accept(SocketHandle listen_socket);
  uint16_t GetBoundPort(SocketHandle socket);

 private:
  bool EnsureWinsock();
  bool winsock_ready_ = false;
};
}
