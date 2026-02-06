#pragma once

#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#endif

class SshSubprocess {
 public:
  SshSubprocess();
  ~SshSubprocess();

  bool Start(const std::wstring& command_line);
  bool Read(std::string* output);
  bool Write(const std::string& data);
  bool IsRunning() const;
  int Wait();
  void Terminate();

 private:
#ifdef _WIN32
  HANDLE process_ = INVALID_HANDLE_VALUE;
  HANDLE thread_ = INVALID_HANDLE_VALUE;
  HANDLE stdin_write_ = INVALID_HANDLE_VALUE;
  HANDLE stdout_read_ = INVALID_HANDLE_VALUE;
#endif
};
