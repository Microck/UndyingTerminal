#pragma once

#include <string>

class WindowsPaths {
 public:
  static std::string GetProgramDataPath();
  static std::string GetLocalAppDataPath();
};
