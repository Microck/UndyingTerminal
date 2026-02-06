#pragma once

#include <string>
#include <thread>

class NamedPipeServer {
 public:
  NamedPipeServer();
  ~NamedPipeServer();

  NamedPipeServer(const NamedPipeServer&) = delete;
  NamedPipeServer& operator=(const NamedPipeServer&) = delete;

  bool Start(class ClientRegistry* registry);
  void Stop();

 private:
 private:
#ifdef _WIN32
  void Run();
  void HandleClient(void* pipe_handle);
  class ClientRegistry* registry_ = nullptr;
  void* stop_event_ = nullptr;
  std::thread worker_;
  bool running_ = false;
  std::wstring pipe_name_;
#endif
};
