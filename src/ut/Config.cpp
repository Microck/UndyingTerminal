#include "Config.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include "WindowsPaths.hpp"

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#endif

namespace {
std::string DefaultConfigPath() {
#ifdef _WIN32
  std::string base = WindowsPaths::GetProgramDataPath();
  if (base.empty()) {
    return "";
  }
  std::string path = base;
  if (!path.empty() && path.back() != '\\' && path.back() != '/') {
    path += "\\";
  }
  path += "UndyingTerminal\\ut.cfg";
  return path;
#else
  return "ut.cfg";
#endif
}
}  // namespace

void Config::Load() {
  config_path = DefaultConfigPath();
  if (config_path.empty()) {
    return;
  }

#ifdef _WIN32
  const size_t pos = config_path.find_last_of("\\/");
  if (pos != std::string::npos) {
    const std::string dir = config_path.substr(0, pos);
    SHCreateDirectoryExA(nullptr, dir.c_str(), nullptr);
  }
#endif

  std::ifstream input(config_path);
  if (!input.is_open()) {
    std::ofstream output(config_path);
    if (output.is_open()) {
      output << "port=2022\n";
      output << "bind_ip=0.0.0.0\n";
      output << "verbose=0\n";
      output << "silent=0\n";
      output << "logsize=20971520\n";
      output << "logdirectory=/tmp\n";
      output << "telemetry=true\n";
    }
    return;
  }

  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    const auto pos = line.find('=');
    if (pos == std::string::npos) {
      continue;
    }
    const std::string key = line.substr(0, pos);
    const std::string value = line.substr(pos + 1);
    if (key == "port") {
      std::istringstream stream(value);
      int parsed = 0;
      if (stream >> parsed) {
        port = parsed;
      }
    } else if (key == "bind_ip") {
      this->bind_ip = value;
    } else if (key == "verbose") {
      this->verbose = value == "1" || value == "true";
    } else if (key == "silent") {
      this->silent = value == "1" || value == "true";
    } else if (key == "logsize") {
      std::istringstream stream(value);
      int parsed = 0;
      if (stream >> parsed) {
        this->logsize = parsed;
      }
    } else if (key == "logdirectory") {
      this->logdirectory = value;
    } else if (key == "telemetry") {
      this->telemetry = value == "1" || value == "true";
    } else if (key == "shared_key") {
      shared_key_hex = value;
    }
  }
}
