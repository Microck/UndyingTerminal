#pragma once

#include <string>

class SshCommandBuilder {
 public:
  SshCommandBuilder& SetHost(const std::string& host);
  SshCommandBuilder& SetUser(const std::string& user);
  SshCommandBuilder& SetPort(int port);
  SshCommandBuilder& SetIdentityFile(const std::string& path);
  SshCommandBuilder& SetRemoteCommand(const std::string& command);
  SshCommandBuilder& AddOption(const std::string& option);

  std::wstring Build() const;

 private:
  std::string host_;
  std::string user_;
  int port_ = 22;
  std::string identity_file_;
  std::string remote_command_;
  std::string extra_options_;
};
