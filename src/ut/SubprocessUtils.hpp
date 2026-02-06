#pragma once

#include <string>

class SubprocessUtils {
 public:
  static bool RunAndWait(const std::wstring& command_line, unsigned long* exit_code);
};
