#pragma once

#include <string>

struct Config {
  int port = 2022;
  std::string bind_ip = "0.0.0.0";
  bool verbose = false;
  bool silent = false;
  int logsize = 20971520;
  std::string logdirectory = "/tmp";
  bool telemetry = true;
  std::string config_path;
  std::string shared_key_hex;

  void Load();
  bool IsVerbose() const { return verbose; }
};
