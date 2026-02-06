#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

class ConPTYSession {
 public:
  ConPTYSession();
  ~ConPTYSession();

  ConPTYSession(const ConPTYSession&) = delete;
  ConPTYSession& operator=(const ConPTYSession&) = delete;

  bool Start(const std::wstring& command_line, bool enable_resize_loop = true);
  void Run();
  void Wait();
  bool IsRunning() const;
#ifdef _WIN32
  HANDLE InputWriteHandle() const;
  HANDLE OutputReadHandle() const;
  void Resize(short cols, short rows);
#endif

 private:
#ifdef _WIN32
  void CloseHandles();
  void StartResizeLoop();
#endif
};
