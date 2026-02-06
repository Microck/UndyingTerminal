#include "SubprocessUtils.hpp"

#include <iostream>

int main() {
  unsigned long exit_code = 1;

#ifdef _WIN32
  const bool ok = SubprocessUtils::RunAndWait(L"cmd.exe /c exit 0", &exit_code);
  if (!ok || exit_code != 0) {
    std::cerr << "SubprocessUtils::RunAndWait failed\n";
    return 1;
  }
#else
  const bool ok = SubprocessUtils::RunAndWait(L"", &exit_code);
  if (!ok || exit_code != 0) {
    std::cerr << "SubprocessUtils stub failed\n";
    return 1;
  }
#endif

  return 0;
}
