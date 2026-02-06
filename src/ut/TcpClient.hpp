#pragma once

#include <cstdint>
#include <string>

class TcpClient {
 public:
  TcpClient();
  ~TcpClient();

  TcpClient(const TcpClient&) = delete;
  TcpClient& operator=(const TcpClient&) = delete;

  bool Connect(const std::string& host, uint16_t port);
  bool Send(const std::string& data);
  bool Receive(std::string* out);
  void Close();
  bool IsConnected() const;
  int last_error() const { return last_error_; }

 private:
#ifdef _WIN32
  unsigned long long socket_ = ~0ULL;
#else
  int socket_ = -1;
#endif
  int last_error_ = 0;
};
