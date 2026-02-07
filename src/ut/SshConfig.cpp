#include "SshConfig.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace {
std::string Trim(const std::string& value) {
  size_t start = 0;
  while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
    ++start;
  }
  size_t end = value.size();
  while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }
  return value.substr(start, end - start);
}

std::string ToLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
  return value;
}

std::vector<std::string> SplitWhitespace(const std::string& value) {
  std::vector<std::string> out;
  std::istringstream stream(value);
  std::string part;
  while (stream >> part) {
    out.push_back(part);
  }
  return out;
}

bool MatchGlob(const std::string& pattern, const std::string& text) {
  size_t p = 0;
  size_t t = 0;
  size_t star = std::string::npos;
  size_t match = 0;
  while (t < text.size()) {
    if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == text[t])) {
      ++p;
      ++t;
      continue;
    }
    if (p < pattern.size() && pattern[p] == '*') {
      star = p++;
      match = t;
      continue;
    }
    if (star != std::string::npos) {
      p = star + 1;
      t = ++match;
      continue;
    }
    return false;
  }
  while (p < pattern.size() && pattern[p] == '*') {
    ++p;
  }
  return p == pattern.size();
}

bool HostMatches(const std::vector<std::string>& patterns, const std::string& host) {
  const std::string lowered_host = ToLower(host);
  for (const auto& raw_pattern : patterns) {
    if (raw_pattern.empty() || raw_pattern[0] == '!') {
      continue;
    }
    const std::string lowered_pattern = ToLower(raw_pattern);
    if (MatchGlob(lowered_pattern, lowered_host)) {
      return true;
    }
  }
  return false;
}

std::string ExpandHome(const std::string& value) {
  if (value.empty() || value[0] != '~') {
    return value;
  }
  const char* home = std::getenv("USERPROFILE");
  if (!home || !*home) {
    home = std::getenv("HOME");
  }
  if (!home || !*home) {
    return value;
  }
  if (value.size() == 1) {
    return std::string(home);
  }
  if (value[1] == '/' || value[1] == '\\') {
    return std::string(home) + value.substr(1);
  }
  return value;
}

bool ParseHostPort(const std::string& input, std::string* host, std::string* port) {
  if (!host || !port) {
    return false;
  }
  if (input.empty()) {
    return false;
  }
  if (input.front() == '[') {
    const auto close = input.find(']');
    if (close == std::string::npos || close + 2 > input.size() || input[close + 1] != ':') {
      return false;
    }
    *host = input.substr(1, close - 1);
    *port = input.substr(close + 2);
    return true;
  }
  const auto colon = input.rfind(':');
  if (colon == std::string::npos) {
    return false;
  }
  const std::string host_part = input.substr(0, colon);
  if (host_part.find(':') != std::string::npos) {
    return false;
  }
  *host = host_part;
  *port = input.substr(colon + 1);
  return true;
}

bool IsNumeric(const std::string& value) {
  if (value.empty()) {
    return false;
  }
  for (char c : value) {
    if (!std::isdigit(static_cast<unsigned char>(c))) {
      return false;
    }
  }
  return true;
}

std::string NormalizeAddress(const std::string& value) {
  if (value.find(':') != std::string::npos && (value.empty() || value.front() != '[')) {
    return "[" + value + "]";
  }
  return value;
}
}  // namespace

std::string SshConfig::DefaultConfigPath() {
  const char* home = std::getenv("USERPROFILE");
  if (!home || !*home) {
    home = std::getenv("HOME");
  }
  if (!home || !*home) {
    return {};
  }
  std::string path(home);
  if (!path.empty() && path.back() != '\\' && path.back() != '/') {
    path.push_back('\\');
  }
  path += ".ssh\\config";
  return path;
}

bool SshConfig::LoadForHost(const std::string& host,
                            const std::string& config_path,
                            bool optional,
                            SshConfigOptions* out,
                            std::string* error) {
  if (!out) {
    if (error) {
      *error = "missing output config";
    }
    return false;
  }
  *out = {};
  if (config_path.empty()) {
    return true;
  }
  std::ifstream file(config_path);
  if (!file.is_open()) {
    if (!optional && error) {
      *error = "SSH config not found: " + config_path;
      return false;
    }
    return true;
  }

  bool seen_host = false;
  bool current_match = true;
  std::string line;
  while (std::getline(file, line)) {
    auto trimmed = Trim(line);
    const auto comment_pos = trimmed.find('#');
    if (comment_pos != std::string::npos) {
      trimmed = Trim(trimmed.substr(0, comment_pos));
    }
    if (trimmed.empty() || trimmed[0] == '#') {
      continue;
    }
    const auto tokens = SplitWhitespace(trimmed);
    if (tokens.empty()) {
      continue;
    }
    const std::string key = ToLower(tokens[0]);
    if (key == "host") {
      seen_host = true;
      std::vector<std::string> patterns(tokens.begin() + 1, tokens.end());
      current_match = HostMatches(patterns, host);
      continue;
    }
    if (!seen_host) {
      current_match = true;
    }
    if (!current_match) {
      continue;
    }
    if (tokens.size() < 2) {
      continue;
    }
    const std::string value = Trim(trimmed.substr(tokens[0].size()));
    if (key == "hostname") {
      out->host_name = value;
    } else if (key == "user") {
      out->user = value;
    } else if (key == "port") {
      if (IsNumeric(tokens[1])) {
        out->port = std::stoi(tokens[1]);
      }
    } else if (key == "identityfile") {
      out->identity_file = ExpandHome(value);
    } else if (key == "proxyjump") {
      std::string normalized = value;
      normalized.erase(std::remove_if(normalized.begin(), normalized.end(), [](unsigned char c) {
                         return std::isspace(c) != 0;
                       }),
                       normalized.end());
      out->proxy_jump = normalized;
    } else if (key == "forwardagent") {
      const std::string lowered = ToLower(tokens[1]);
      out->forward_agent_set = true;
      out->forward_agent = (lowered == "yes" || lowered == "true" || lowered == "on" || lowered == "1");
    } else if (key == "localforward") {
      std::string normalized;
      if (NormalizeLocalForward(value, &normalized)) {
        out->local_forwards.push_back(normalized);
      }
    }
  }
  return true;
}

bool SshConfig::NormalizeLocalForward(const std::string& value, std::string* out) {
  if (!out) {
    return false;
  }
  const auto parts = SplitWhitespace(value);
  if (parts.size() < 2 || parts.size() > 3) {
    return false;
  }
  std::string bind_host;
  std::string bind_port;
  if (parts[0].find(':') != std::string::npos) {
    if (!ParseHostPort(parts[0], &bind_host, &bind_port)) {
      return false;
    }
  } else {
    bind_port = parts[0];
  }
  if (!IsNumeric(bind_port)) {
    return false;
  }
  if (bind_host.empty()) {
    bind_host = "127.0.0.1";
  }

  std::string dest_host;
  std::string dest_port;
  if (parts.size() == 2) {
    if (!ParseHostPort(parts[1], &dest_host, &dest_port)) {
      return false;
    }
  } else {
    dest_host = parts[1];
    dest_port = parts[2];
  }
  if (!IsNumeric(dest_port)) {
    return false;
  }

  const std::string bind_norm = NormalizeAddress(bind_host);
  const std::string dest_norm = NormalizeAddress(dest_host);
  *out = bind_norm + ":" + bind_port + ":" + dest_norm + ":" + dest_port;
  return true;
}
