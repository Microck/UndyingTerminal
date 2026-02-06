#include "NamedPipeClient.hpp"

#ifdef _WIN32
#include <windows.h>

NamedPipeClient::NamedPipeClient() = default;

NamedPipeClient::~NamedPipeClient() {
  Close();
}

bool NamedPipeClient::Connect(const std::wstring& pipe_name) {
  Close();

  HANDLE pipe = CreateFileW(
      pipe_name.c_str(),
      GENERIC_READ | GENERIC_WRITE,
      0,
      nullptr,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      nullptr);

  if (pipe == INVALID_HANDLE_VALUE) {
    return false;
  }

  pipe_ = pipe;
  return true;
}

bool NamedPipeClient::Send(const std::string& data) {
  if (!pipe_) {
    return false;
  }
  DWORD written = 0;
  return WriteFile(static_cast<HANDLE>(pipe_), data.data(), static_cast<DWORD>(data.size()), &written, nullptr) != 0;
}

bool NamedPipeClient::Receive(std::string* out) {
  if (!pipe_ || !out) {
    return false;
  }
  char buffer[256] = {};
  DWORD read_bytes = 0;
  if (!ReadFile(static_cast<HANDLE>(pipe_), buffer, sizeof(buffer) - 1, &read_bytes, nullptr)) {
    return false;
  }
  out->assign(buffer, buffer + read_bytes);
  return true;
}

void NamedPipeClient::Close() {
  if (pipe_) {
    CloseHandle(static_cast<HANDLE>(pipe_));
    pipe_ = nullptr;
  }
}
#else
NamedPipeClient::NamedPipeClient() = default;
NamedPipeClient::~NamedPipeClient() = default;

bool NamedPipeClient::Connect(const std::wstring& pipe_name) {
  (void)pipe_name;
  return true;
}

bool NamedPipeClient::Send(const std::string& data) {
  (void)data;
  return true;
}

bool NamedPipeClient::Receive(std::string* out) {
  if (out) {
    out->clear();
  }
  return true;
}

void NamedPipeClient::Close() {}
#endif
