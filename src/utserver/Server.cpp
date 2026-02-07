#include "Server.hpp"

#include <array>
#include <iostream>
#include <string>

#include "Verbose.hpp"
#include "protocol/SocketTypes.hpp"

#ifdef UNDYING_TERMINAL_REQUIRE_DEPS
#include <sodium.h>
#endif

Server::Server() = default;

Server::~Server() {
  Stop();
}

bool Server::Start(uint16_t port, const std::string& bind_ip) {
  if (running_) {
    return true;
  }
  
  if (!pipe_server_.Start(&registry_)) {
    if (IsVerbose()) {
      std::cerr << "Named pipe server failed to start\n";
    }
    return false;
  }
  
  if (!job_object_.Create()) {
    if (IsVerbose()) {
      std::cerr << "Job object failed to create\n";
    }
    pipe_server_.Stop();
    return false;
  }

#ifdef UNDYING_TERMINAL_REQUIRE_DEPS
  if (sodium_init() < 0) {
    if (IsVerbose()) {
      std::cerr << "sodium_init failed\n";
    }
    pipe_server_.Stop();
    return false;
  }
#endif

  if (!tcp_listener_.Start(port, bind_ip, &registry_)) {
    if (IsVerbose()) {
      std::cerr << "TcpListener failed to start\n";
    }
    pipe_server_.Stop();
    return false;
  }

  running_ = true;
  registry_.RegisterTerminal("self-test", "", ut::kInvalidSocket);
  return true;
}

void Server::SetSharedKey(const std::array<unsigned char, 32>& key) {
  shared_key_ = key;
  encryption_enabled_ = true;
  tcp_listener_.SetSharedKey(shared_key_);
}

void Server::Stop() {
  if (!running_) {
    return;
  }

  tcp_listener_.Stop();
  pipe_server_.Stop();
  registry_.UnregisterTerminal("self-test");
  job_object_.Close();
  running_ = false;
}
#include "Verbose.hpp"
