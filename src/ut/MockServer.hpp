#pragma once

#include <cstdint>
#include <thread>

class MockServer {
 public:
  MockServer();
  ~MockServer();

  MockServer(const MockServer&) = delete;
  MockServer& operator=(const MockServer&) = delete;

  bool Start(uint16_t* port_out);
  void Stop();

 private:
  void AcceptLoop();

#ifdef _WIN32
  unsigned long long listen_socket_ = ~0ULL;
  unsigned long long client_socket_ = ~0ULL;
#else
  int listen_socket_ = -1;
  int client_socket_ = -1;
#endif
  std::thread accept_thread_;
  bool running_ = false;
};
