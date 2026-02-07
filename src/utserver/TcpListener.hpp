#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include "protocol/SocketTypes.hpp"
#include "protocol/TcpSocketHandler.hpp"

class TcpListener {
public:
  TcpListener();
  ~TcpListener();

  TcpListener(const TcpListener&) = delete;
  TcpListener& operator=(const TcpListener&) = delete;

  bool Start(uint16_t port, const std::string& bind_ip, class ClientRegistry* registry);
  void Stop();
  uint16_t port() const { return port_; }
  void SetSharedKey(const std::array<unsigned char, 32>& key);

private:
  void AcceptLoop();
  void HandleClient(ut::SocketHandle client);

  ut::SocketHandle listen_socket_ = ut::kInvalidSocket;
  std::thread accept_thread_;
  bool running_ = false;
  uint16_t port_ = 0;
  class ClientRegistry* registry_ = nullptr;
  bool encryption_enabled_ = false;
  std::array<unsigned char, 32> shared_key_{};
  std::shared_ptr<ut::TcpSocketHandler> socket_handler_;
};
