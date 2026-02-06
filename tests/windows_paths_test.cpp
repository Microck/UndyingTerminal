#include "WindowsPaths.hpp"

#include <iostream>

int main() {
#ifdef _WIN32
  if (WindowsPaths::GetProgramDataPath().empty()) {
    std::cerr << "ProgramData path missing\n";
    return 1;
  }
  if (WindowsPaths::GetLocalAppDataPath().empty()) {
    std::cerr << "LocalAppData path missing\n";
    return 1;
  }
#endif
  return 0;
}
