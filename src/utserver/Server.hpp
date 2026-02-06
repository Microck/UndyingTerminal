#pragma once

#include "ClientRegistry.hpp"
#include "JobObject.hpp"
#include "NamedPipeServer.hpp"
#include "TcpListener.hpp"

#include <array>
#include <string>

class Server {
 public:
  Server();
  ~Server();

  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;

  bool Start(uint16_t port = 2022, const std::string& bind_ip = "");
  void Stop();
  uint16_t port() const { return tcp_listener_.port(); }
  void SetSharedKey(const std::array<unsigned char, 32>& key);

 private:
  ClientRegistry registry_;
  JobObject job_object_;
  NamedPipeServer pipe_server_;
  TcpListener tcp_listener_;
  bool running_ = false;
  bool encryption_enabled_ = false;
  std::array<unsigned char, 32> shared_key_{};
};
