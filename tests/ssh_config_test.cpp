#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "SshConfig.hpp"

int main() {
  const char* temp_dir = std::getenv("TEMP");
  if (!temp_dir || !*temp_dir) {
    temp_dir = std::getenv("TMP");
  }
  std::string temp_path = temp_dir && *temp_dir ? std::string(temp_dir) : ".";
  if (!temp_path.empty() && temp_path.back() != '\\' && temp_path.back() != '/') {
    temp_path.push_back('\\');
  }
  temp_path += "undying-terminal-ssh-config-test";
  std::ofstream out(temp_path);
  if (!out.is_open()) {
    std::cerr << "Failed to open temp config file\n";
    return 1;
  }
  out << "Host *\n";
  out << "  User default\n";
  out << "  Port 22\n";
  out << "Host test\n";
  out << "  HostName example.com\n";
  out << "  User alice\n";
  out << "  Port 2222\n";
  out << "  IdentityFile C:/keys/id_test\n";
  out << "  ProxyJump jump.example.com\n";
  out << "  LocalForward 8080 example.net:80\n";
  out << "  ForwardAgent yes\n";
  out.close();

  SshConfigOptions options;
  std::string error;
  if (!SshConfig::LoadForHost("test", temp_path, false, &options, &error)) {
    std::cerr << "Failed to load config: " << error << "\n";
    return 1;
  }

  if (options.host_name != "example.com") {
    std::cerr << "HostName not applied\n";
    return 1;
  }
  if (options.user != "alice") {
    std::cerr << "User not applied\n";
    return 1;
  }
  if (options.port != 2222) {
    std::cerr << "Port not applied\n";
    return 1;
  }
  if (options.identity_file != "C:/keys/id_test") {
    std::cerr << "IdentityFile not applied\n";
    return 1;
  }
  if (options.proxy_jump != "jump.example.com") {
    std::cerr << "ProxyJump not applied\n";
    return 1;
  }
  if (!options.forward_agent_set || !options.forward_agent) {
    std::cerr << "ForwardAgent not applied\n";
    return 1;
  }
  if (options.local_forwards.empty()) {
    std::cerr << "LocalForward not applied\n";
    return 1;
  }

  std::string normalized;
  if (!SshConfig::NormalizeLocalForward("8080 example.net:80", &normalized)) {
    std::cerr << "LocalForward normalization failed\n";
    return 1;
  }
  if (normalized != "127.0.0.1:8080:example.net:80") {
    std::cerr << "LocalForward normalization mismatch: " << normalized << "\n";
    return 1;
  }

  std::cout << "SSH config test passed\n";
  return 0;
}
