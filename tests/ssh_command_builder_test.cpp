#include <iostream>
#include <string>

#include "SshCommandBuilder.hpp"

int main() {
  SshCommandBuilder builder;
  builder.SetHost("example.com")
      .SetUser("admin")
      .SetPort(2222)
      .SetRemoteCommand("undying-terminal-terminal.exe");

  std::wstring cmd = builder.Build();
  std::string scmd(cmd.begin(), cmd.end());

  if (scmd.find("ssh.exe") == std::string::npos) {
    std::cerr << "Missing ssh.exe\n";
    return 1;
  }
  if (scmd.find("-p 2222") == std::string::npos) {
    std::cerr << "Missing port\n";
    return 1;
  }
  if (scmd.find("admin@example.com") == std::string::npos) {
    std::cerr << "Missing user@host\n";
    return 1;
  }
  if (scmd.find("undying-terminal-terminal.exe") == std::string::npos) {
    std::cerr << "Missing remote command\n";
    return 1;
  }

  std::cout << "SSH command builder test passed\n";
  std::cout << "Generated: " << scmd << "\n";
  return 0;
}
