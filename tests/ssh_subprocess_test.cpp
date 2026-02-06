#include <iostream>
#include <string>

#include "SshSubprocess.hpp"

int main() {
  SshSubprocess proc;
  if (!proc.Start(L"cmd.exe /c echo hello")) {
    std::cerr << "Failed to start process\n";
    return 1;
  }

  std::string output;
  if (!proc.Read(&output)) {
    std::cerr << "Failed to read output\n";
    return 1;
  }

  if (output.find("hello") == std::string::npos) {
    std::cerr << "Unexpected output: " << output << "\n";
    return 1;
  }

  proc.Wait();

  SshSubprocess proc2;
  if (!proc2.Start(L"cmd.exe")) {
    std::cerr << "Failed to start cmd.exe\n";
    return 1;
  }

  if (!proc2.Write("echo test\r\n")) {
    std::cerr << "Failed to write\n";
    return 1;
  }

  proc2.Terminate();

  std::cout << "SSH subprocess test passed\n";
  return 0;
}
