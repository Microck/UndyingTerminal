#include "PipeSocketHandler.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace ut {
bool PipeSocketHandler::HasData(SocketHandle socket) {
#ifdef _WIN32
  if (socket == kInvalidSocket) {
    return false;
  }
  DWORD available = 0;
  if (!PeekNamedPipe(reinterpret_cast<HANDLE>(socket), nullptr, 0, nullptr, &available, nullptr)) {
    return false;
  }
  return available > 0;
#else
  (void)socket;
  return false;
#endif
}

bool PipeSocketHandler::IsConnected(SocketHandle socket) {
#ifdef _WIN32
  if (socket == kInvalidSocket) {
    return false;
  }
  DWORD available = 0;
  if (PeekNamedPipe(reinterpret_cast<HANDLE>(socket), nullptr, 0, nullptr, &available, nullptr)) {
    return true;
  }
  const DWORD err = GetLastError();
  return err != ERROR_BROKEN_PIPE && err != ERROR_PIPE_NOT_CONNECTED && err != ERROR_INVALID_HANDLE;
#else
  (void)socket;
  return false;
#endif
}

int PipeSocketHandler::Read(SocketHandle socket, void* buf, size_t count) {
#ifdef _WIN32
  if (socket == kInvalidSocket) {
    return -1;
  }
  DWORD read_bytes = 0;
  if (!ReadFile(reinterpret_cast<HANDLE>(socket), buf, static_cast<DWORD>(count), &read_bytes, nullptr)) {
    return -1;
  }
  return static_cast<int>(read_bytes);
#else
  (void)socket;
  (void)buf;
  (void)count;
  return -1;
#endif
}

int PipeSocketHandler::Write(SocketHandle socket, const void* buf, size_t count) {
#ifdef _WIN32
  if (socket == kInvalidSocket) {
    return -1;
  }
  DWORD written = 0;
  if (!WriteFile(reinterpret_cast<HANDLE>(socket), buf, static_cast<DWORD>(count), &written, nullptr)) {
    return -1;
  }
  return static_cast<int>(written);
#else
  (void)socket;
  (void)buf;
  (void)count;
  return -1;
#endif
}

void PipeSocketHandler::Close(SocketHandle socket) {
#ifdef _WIN32
  if (socket != kInvalidSocket) {
    CloseHandle(reinterpret_cast<HANDLE>(socket));
  }
#else
  (void)socket;
#endif
}

SocketHandle PipeSocketHandler::Connect(const std::wstring& pipe_name) {
#ifdef _WIN32
  HANDLE pipe = CreateFileW(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  if (pipe == INVALID_HANDLE_VALUE) {
    return kInvalidSocket;
  }
  return reinterpret_cast<SocketHandle>(pipe);
#else
  (void)pipe_name;
  return kInvalidSocket;
#endif
}
}
