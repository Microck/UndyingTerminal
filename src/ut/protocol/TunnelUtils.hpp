#pragma once

#include <string>
#include <vector>

#include "UTerminal.pb.h"

namespace ut {
class TunnelParseException : public std::exception {
 public:
  explicit TunnelParseException(const std::string& msg) : message_(msg) {}
  const char* what() const noexcept override { return message_.c_str(); }

 private:
  std::string message_;
};

std::vector<ut::PortForwardSourceRequest> ParseRangesToRequests(const std::string& input);
std::vector<std::string> ParseSshTunnelArg(const std::string& input);
}
