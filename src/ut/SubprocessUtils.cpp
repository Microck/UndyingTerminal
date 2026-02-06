#include "SubprocessUtils.hpp"

#ifdef _WIN32
#include <windows.h>

#include <vector>

bool SubprocessUtils::RunAndWait(const std::wstring& command_line, unsigned long* exit_code) {
  if (command_line.empty()) {
    return false;
  }

  std::vector<wchar_t> mutable_cmd(command_line.begin(), command_line.end());
  mutable_cmd.push_back(L'\0');

  STARTUPINFOW startup_info{};
  startup_info.cb = sizeof(startup_info);
  PROCESS_INFORMATION process_info{};

  const BOOL created = CreateProcessW(
      nullptr,
      mutable_cmd.data(),
      nullptr,
      nullptr,
      FALSE,
      0,
      nullptr,
      nullptr,
      &startup_info,
      &process_info);

  if (!created) {
    return false;
  }

  WaitForSingleObject(process_info.hProcess, INFINITE);

  DWORD code = 0;
  if (!GetExitCodeProcess(process_info.hProcess, &code)) {
    code = 1;
  }

  if (exit_code) {
    *exit_code = code;
  }

  CloseHandle(process_info.hThread);
  CloseHandle(process_info.hProcess);
  return code == 0;
}
#else
bool SubprocessUtils::RunAndWait(const std::wstring& command_line, unsigned long* exit_code) {
  (void)command_line;
  if (exit_code) {
    *exit_code = 0;
  }
  return true;
}
#endif
