#include "NamedPipeServer.hpp"

#ifdef _WIN32
#include <windows.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "ClientRegistry.hpp"
#include "protocol/PipeSocketHandler.hpp"
#include "UTerminal.pb.h"
#include "Verbose.hpp"

namespace {
std::wstring GetPipeName() {
  const char* env = std::getenv("UT_PIPE_NAME");
  if (env && *env) {
    return std::wstring(env, env + std::strlen(env));
  }
  return L"\\\\.\\pipe\\undying-terminal";
}
}

NamedPipeServer::NamedPipeServer() = default;

NamedPipeServer::~NamedPipeServer() {
  Stop();
}

bool NamedPipeServer::Start(ClientRegistry* registry) {
  Stop();

  registry_ = registry;
  pipe_name_ = GetPipeName();

  stop_event_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  if (!stop_event_) {
    if (IsVerbose()) {
      std::cerr << "CreateEvent failed: " << GetLastError() << "\n";
    }
    return false;
  }

  running_ = true;
  worker_ = std::thread(&NamedPipeServer::Run, this);

  return true;
}

void NamedPipeServer::Stop() {
  running_ = false;
  if (stop_event_) {
    SetEvent(static_cast<HANDLE>(stop_event_));
  }
  HANDLE cancel_pipe = CreateFileW(
      pipe_name_.empty() ? L"\\\\.\\pipe\\undying-terminal" : pipe_name_.c_str(),
      GENERIC_READ | GENERIC_WRITE,
      0,
      nullptr,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      nullptr);
  if (cancel_pipe != INVALID_HANDLE_VALUE) {
    CloseHandle(cancel_pipe);
  }
  if (worker_.joinable()) {
    worker_.join();
  }
  if (stop_event_) {
    CloseHandle(static_cast<HANDLE>(stop_event_));
    stop_event_ = nullptr;
  }
  registry_ = nullptr;
}

void NamedPipeServer::Run() {
  while (running_) {
    if (WaitForSingleObject(static_cast<HANDLE>(stop_event_), 0) == WAIT_OBJECT_0) {
      break;
    }

    HANDLE pipe = CreateNamedPipeW(
        pipe_name_.empty() ? L"\\\\.\\pipe\\undying-terminal" : pipe_name_.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        4096,
        4096,
        0,
        nullptr);

    if (pipe == INVALID_HANDLE_VALUE) {
      if (IsVerbose()) {
        std::cerr << "CreateNamedPipe failed: " << GetLastError() << "\n";
      }
      break;
    }

    if (WaitForSingleObject(static_cast<HANDLE>(stop_event_), 0) == WAIT_OBJECT_0) {
      CloseHandle(pipe);
      break;
    }

    BOOL connected = ConnectNamedPipe(pipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (connected) {
      HandleClient(pipe);
    } else {
      CloseHandle(pipe);
    }
  }
}

void NamedPipeServer::HandleClient(void* pipe_handle) {
  HANDLE pipe = static_cast<HANDLE>(pipe_handle);
  if (!registry_) {
    CloseHandle(pipe);
    return;
  }

  ut::PipeSocketHandler pipe_handler;
  ut::Packet packet;
  if (!pipe_handler.ReadPacket(reinterpret_cast<ut::SocketHandle>(pipe), &packet)) {
    CloseHandle(pipe);
    return;
  }
  if (packet.header() != static_cast<uint8_t>(ut::TERMINAL_USER_INFO)) {
    CloseHandle(pipe);
    return;
  }
  ut::TerminalUserInfo info;
  if (!info.ParseFromString(packet.payload())) {
    CloseHandle(pipe);
    return;
  }

  registry_->RegisterTerminal(info.id(), info.passkey(), reinterpret_cast<ut::SocketHandle>(pipe_handle));
}
#else
NamedPipeServer::NamedPipeServer() = default;
NamedPipeServer::~NamedPipeServer() = default;

bool NamedPipeServer::Start(ClientRegistry* registry) {
  (void)registry;
  return true;
}

void NamedPipeServer::Stop() {}
#endif
