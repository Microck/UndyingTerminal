#include "SshCommandBuilder.hpp"

#include <cctype>
#include <sstream>

namespace {
bool HasUnsafeChar(const std::string& value) {
  for (unsigned char ch : value) {
    if (std::isspace(ch) || ch == '"') {
      return true;
    }
  }
  return false;
}
}  // namespace

SshCommandBuilder& SshCommandBuilder::SetHost(const std::string& host) {
  host_ = host;
  return *this;
}

SshCommandBuilder& SshCommandBuilder::SetUser(const std::string& user) {
  user_ = user;
  return *this;
}

SshCommandBuilder& SshCommandBuilder::SetPort(int port) {
  port_ = port;
  return *this;
}

SshCommandBuilder& SshCommandBuilder::SetIdentityFile(const std::string& path) {
  identity_file_ = path;
  return *this;
}

SshCommandBuilder& SshCommandBuilder::SetRemoteCommand(const std::string& command) {
  remote_command_ = command;
  return *this;
}

  SshCommandBuilder& SshCommandBuilder::AddOption(const std::string& option) {
   if (!extra_options_.empty()) {
     extra_options_ += " ";
   }
  extra_options_ += option;
   return *this;
}

std::wstring SshCommandBuilder::Build() const {
  if (host_.empty() || HasUnsafeChar(host_) || HasUnsafeChar(user_)) {
    return L"";
  }
  if (identity_file_.find('"') != std::string::npos || remote_command_.find('"') != std::string::npos) {
    return L"";
  }

  std::ostringstream cmd;
  cmd << "ssh.exe";

  if (port_ != 22) {
    cmd << " -p " << port_;
  }

  if (!identity_file_.empty()) {
    cmd << " -i \"" << identity_file_ << "\"";
  }

  if (!extra_options_.empty()) {
    cmd << " " << extra_options_;
  }

  if (!user_.empty()) {
    cmd << " " << user_ << "@" << host_;
  } else {
    cmd << " " << host_;
  }

  if (!remote_command_.empty()) {
    cmd << " \"" << remote_command_ << "\"";
  }

  const std::string result = cmd.str();
  return std::wstring(result.begin(), result.end());
}
