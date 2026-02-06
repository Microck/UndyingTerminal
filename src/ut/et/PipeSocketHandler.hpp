#pragma once

#include <string>

#include "SocketHandler.hpp"

namespace ut {
class PipeSocketHandler : public SocketHandler {
 public:
  PipeSocketHandler() = default;
  ~PipeSocketHandler() override = default;

  bool HasData(SocketHandle socket) override;
  int Read(SocketHandle socket, void* buf, size_t count) override;
  int Write(SocketHandle socket, const void* buf, size_t count) override;
  void Close(SocketHandle socket) override;

  SocketHandle Connect(const std::wstring& pipe_name);
};
}
