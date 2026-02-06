#include "WindowsPaths.hpp"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>

#include <string>

namespace {
std::string WideToUtf8(const wchar_t* value) {
  if (!value) {
    return {};
  }
  const int required = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
  if (required <= 1) {
    return {};
  }
  std::string result(static_cast<size_t>(required - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), required, nullptr, nullptr);
  return result;
}

std::string GetKnownFolderPath(REFKNOWNFOLDERID folder_id) {
  PWSTR path = nullptr;
  const HRESULT hr = SHGetKnownFolderPath(folder_id, KF_FLAG_DEFAULT, nullptr, &path);
  if (FAILED(hr) || !path) {
    return {};
  }
  std::string result = WideToUtf8(path);
  CoTaskMemFree(path);
  return result;
}
}  // namespace

std::string WindowsPaths::GetProgramDataPath() {
  return GetKnownFolderPath(FOLDERID_ProgramData);
}

std::string WindowsPaths::GetLocalAppDataPath() {
  return GetKnownFolderPath(FOLDERID_LocalAppData);
}
#else
std::string WindowsPaths::GetProgramDataPath() {
  return {};
}

std::string WindowsPaths::GetLocalAppDataPath() {
  return {};
}
#endif
