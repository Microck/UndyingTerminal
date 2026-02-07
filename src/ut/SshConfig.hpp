#pragma once

#include <string>
#include <vector>

struct SshConfigOptions {
  std::string host_name;
  std::string user;
  std::string identity_file;
  std::string proxy_jump;
  int port = 0;
  bool forward_agent_set = false;
  bool forward_agent = false;
  std::vector<std::string> local_forwards;
};

class SshConfig {
 public:
  static std::string DefaultConfigPath();
  static bool LoadForHost(const std::string& host,
                          const std::string& config_path,
                          bool optional,
                          SshConfigOptions* out,
                          std::string* error);
  static bool NormalizeLocalForward(const std::string& value, std::string* out);
};
