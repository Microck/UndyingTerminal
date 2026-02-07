#include "TunnelUtils.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace ut {
namespace {
std::vector<std::string> Split(const std::string& input, char delim) {
  std::vector<std::string> out;
  std::string current;
  std::istringstream stream(input);
  while (std::getline(stream, current, delim)) {
    out.push_back(current);
  }
  return out;
}

bool IsNumericOrRange(const std::string& input) {
  for (char c : input) {
    if (!(std::isdigit(static_cast<unsigned char>(c)) || c == '-')) {
      return false;
    }
  }
  return true;
}

void ProcessUtStyleArg(std::vector<ut::PortForwardSourceRequest>& out,
                       const std::vector<std::string>& source_dest,
                       const std::string& input) {
  if (source_dest.size() < 2) {
    throw TunnelParseException("Tunnel argument must have source and destination between a ':'");
  }

  const std::string& source = source_dest[0];
  const std::string& dest = source_dest[1];

  if (!IsNumericOrRange(source) && !IsNumericOrRange(dest)) {
    ut::PortForwardSourceRequest req;
    req.set_environmentvariable(source);
    req.mutable_destination()->set_name(dest);
    out.push_back(req);
    return;
  }

  const bool source_range = source.find('-') != std::string::npos;
  const bool dest_range = dest.find('-') != std::string::npos;
  if (source_range && dest_range) {
    auto source_parts = Split(source, '-');
    auto dest_parts = Split(dest, '-');
    int source_start = std::stoi(source_parts[0]);
    int source_end = std::stoi(source_parts[1]);
    int dest_start = std::stoi(dest_parts[0]);
    int dest_end = std::stoi(dest_parts[1]);
    if (source_end - source_start != dest_end - dest_start) {
      throw TunnelParseException("source/destination port range must have same length");
    }
    const int length = source_end - source_start + 1;
    for (int i = 0; i < length; ++i) {
      ut::PortForwardSourceRequest req;
      req.mutable_source()->set_name("localhost");
      req.mutable_source()->set_port(source_start + i);
      req.mutable_destination()->set_port(dest_start + i);
      out.push_back(req);
    }
    return;
  }

  if (source_range || dest_range) {
    throw TunnelParseException(
        "Invalid port range syntax: if source is a range, destination must be a range (and vice versa)");
  }

  ut::PortForwardSourceRequest req;
  req.mutable_source()->set_name("localhost");
  req.mutable_source()->set_port(std::stoi(source));
  req.mutable_destination()->set_port(std::stoi(dest));
  out.push_back(req);
}
}

std::vector<std::string> ParseSshTunnelArg(const std::string& input) {
  const char colon = ':';
  const char l_bracket = '[';
  const char r_bracket = ']';

  bool in_brackets = false;
  std::string current;
  std::vector<std::string> parts;

  for (char c : input) {
    if (c == l_bracket) {
      in_brackets = true;
      continue;
    }
    if (c == r_bracket) {
      in_brackets = false;
      continue;
    }
    if (c == colon && !in_brackets) {
      parts.push_back(current);
      current.clear();
      continue;
    }
    current.push_back(c);
  }
  parts.push_back(current);
  if (parts.size() < 4) {
    throw TunnelParseException(
        "The 4 part ssh-style tunneling arg (bind_address:port:host:hostport) must be supplied.");
  }
  if (parts.size() > 4) {
    throw TunnelParseException(
        "Ipv6 addresses must be inside of square brackets, ie [::1]:8080:[::]:9090");
  }
  return parts;
}

std::vector<ut::PortForwardSourceRequest> ParseRangesToRequests(const std::string& input) {
  std::vector<ut::PortForwardSourceRequest> out;
  auto comma_parts = Split(input, ',');
  if (comma_parts.size() > 1) {
    for (const auto& part : comma_parts) {
      auto source_dest = Split(part, ':');
      ProcessUtStyleArg(out, source_dest, input);
    }
    return out;
  }

  auto source_dest = Split(input, ':');
  if (source_dest.size() <= 2) {
    ProcessUtStyleArg(out, source_dest, input);
    return out;
  }

  auto ssh_parts = ParseSshTunnelArg(input);
  ut::PortForwardSourceRequest req;
  req.mutable_source()->set_name(ssh_parts[0]);
  req.mutable_source()->set_port(std::stoi(ssh_parts[1]));
  req.mutable_destination()->set_name(ssh_parts[2]);
  req.mutable_destination()->set_port(std::stoi(ssh_parts[3]));
  out.push_back(req);
  return out;
}
}
