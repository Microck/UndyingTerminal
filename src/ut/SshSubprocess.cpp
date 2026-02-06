#include "SshSubprocess.hpp"

#ifdef _WIN32
#include <vector>

SshSubprocess::SshSubprocess() = default;

SshSubprocess::~SshSubprocess() {
  Terminate();
}

bool SshSubprocess::Start(const std::wstring& command_line) {
  if (command_line.empty()) {
    return false;
  }

  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;

  HANDLE stdin_read = INVALID_HANDLE_VALUE;
  HANDLE stdout_write = INVALID_HANDLE_VALUE;

  if (!CreatePipe(&stdin_read, &stdin_write_, &sa, 0)) {
    return false;
  }
  if (!CreatePipe(&stdout_read_, &stdout_write, &sa, 0)) {
    CloseHandle(stdin_read);
    CloseHandle(stdin_write_);
    stdin_write_ = INVALID_HANDLE_VALUE;
    return false;
  }

  SetHandleInformation(stdin_write_, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(stdout_read_, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOW startup_info{};
  startup_info.cb = sizeof(startup_info);
  startup_info.dwFlags = STARTF_USESTDHANDLES;
  startup_info.hStdInput = stdin_read;
  startup_info.hStdOutput = stdout_write;
  startup_info.hStdError = stdout_write;

  PROCESS_INFORMATION process_info{};
  std::vector<wchar_t> mutable_cmd(command_line.begin(), command_line.end());
  mutable_cmd.push_back(L'\0');

  const BOOL created = CreateProcessW(
      nullptr,
      mutable_cmd.data(),
      nullptr,
      nullptr,
      TRUE,
      CREATE_NO_WINDOW,
      nullptr,
      nullptr,
      &startup_info,
      &process_info);

  CloseHandle(stdin_read);
  CloseHandle(stdout_write);

  if (!created) {
    CloseHandle(stdin_write_);
    CloseHandle(stdout_read_);
    stdin_write_ = INVALID_HANDLE_VALUE;
    stdout_read_ = INVALID_HANDLE_VALUE;
    return false;
  }

  process_ = process_info.hProcess;
  thread_ = process_info.hThread;
  return true;
}

bool SshSubprocess::Read(std::string* output) {
  if (!output || stdout_read_ == INVALID_HANDLE_VALUE) {
    return false;
  }

  char buffer[4096] = {};
  DWORD bytes_read = 0;
  if (!ReadFile(stdout_read_, buffer, sizeof(buffer), &bytes_read, nullptr) || bytes_read == 0) {
    return false;
  }

  output->assign(buffer, buffer + bytes_read);
  return true;
}

bool SshSubprocess::Write(const std::string& data) {
  if (stdin_write_ == INVALID_HANDLE_VALUE) {
    return false;
  }

  DWORD written = 0;
  if (!WriteFile(stdin_write_, data.data(), static_cast<DWORD>(data.size()), &written, nullptr)) {
    return false;
  }

  return written == static_cast<DWORD>(data.size());
}

bool SshSubprocess::IsRunning() const {
  if (process_ == INVALID_HANDLE_VALUE) {
    return false;
  }

  DWORD code = 0;
  if (!GetExitCodeProcess(process_, &code)) {
    return false;
  }

  return code == STILL_ACTIVE;
}

int SshSubprocess::Wait() {
  if (process_ == INVALID_HANDLE_VALUE) {
    return -1;
  }

  WaitForSingleObject(process_, INFINITE);
  DWORD code = 0;
  GetExitCodeProcess(process_, &code);
  return static_cast<int>(code);
}

void SshSubprocess::Terminate() {
  if (process_ != INVALID_HANDLE_VALUE) {
    TerminateProcess(process_, 1);
    CloseHandle(process_);
    process_ = INVALID_HANDLE_VALUE;
  }

  if (thread_ != INVALID_HANDLE_VALUE) {
    CloseHandle(thread_);
    thread_ = INVALID_HANDLE_VALUE;
  }

  if (stdin_write_ != INVALID_HANDLE_VALUE) {
    CloseHandle(stdin_write_);
    stdin_write_ = INVALID_HANDLE_VALUE;
  }

  if (stdout_read_ != INVALID_HANDLE_VALUE) {
    CloseHandle(stdout_read_);
    stdout_read_ = INVALID_HANDLE_VALUE;
  }
}
#else
SshSubprocess::SshSubprocess() = default;
SshSubprocess::~SshSubprocess() = default;

bool SshSubprocess::Start(const std::wstring&) {
  return false;
}

bool SshSubprocess::Read(std::string*) {
  return false;
}

bool SshSubprocess::Write(const std::string&) {
  return false;
}

bool SshSubprocess::IsRunning() const {
  return false;
}

int SshSubprocess::Wait() {
  return -1;
}

void SshSubprocess::Terminate() {}
#endif
