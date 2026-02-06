#pragma once

#include <string>

class NamedPipeClient {
 public:
  NamedPipeClient();
  ~NamedPipeClient();

  NamedPipeClient(const NamedPipeClient&) = delete;
  NamedPipeClient& operator=(const NamedPipeClient&) = delete;

  bool Connect(const std::wstring& pipe_name);
  bool Send(const std::string& data);
  bool Receive(std::string* out);
  void Close();

 private:
#ifdef _WIN32
  void* pipe_ = nullptr;
#endif
};
