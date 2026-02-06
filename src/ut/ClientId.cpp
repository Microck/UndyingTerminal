#include "ClientId.hpp"

#include <iomanip>
#include <random>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

const char* ClientId::kRegistryKey = "SOFTWARE\\UndyingTerminal";
const char* ClientId::kValueName = "ClientId";

std::string ClientId::Generate() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

  std::ostringstream ss;
  ss << std::hex << std::setfill('0');
  ss << std::setw(8) << dist(gen);
  ss << "-";
  ss << std::setw(4) << (dist(gen) & 0xFFFF);
  ss << "-";
  ss << std::setw(4) << (dist(gen) & 0xFFFF);
  ss << "-";
  ss << std::setw(8) << dist(gen);
  return ss.str();
}

#ifdef _WIN32
std::string ClientId::GetStored() {
  HKEY key = nullptr;
  if (RegOpenKeyExA(HKEY_CURRENT_USER, kRegistryKey, 0, KEY_READ, &key) != ERROR_SUCCESS) {
    return "";
  }

  char buffer[256] = {};
  DWORD size = sizeof(buffer) - 1;
  DWORD type = REG_SZ;
  LSTATUS status = RegQueryValueExA(
      key,
      kValueName,
      nullptr,
      &type,
      reinterpret_cast<LPBYTE>(buffer),
      &size);
  RegCloseKey(key);

  if (status != ERROR_SUCCESS) {
    return "";
  }

  return std::string(buffer);
}

bool ClientId::Store(const std::string& id) {
  HKEY key = nullptr;
  if (RegCreateKeyExA(HKEY_CURRENT_USER, kRegistryKey, 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr) !=
      ERROR_SUCCESS) {
    return false;
  }

  LSTATUS status = RegSetValueExA(
      key,
      kValueName,
      0,
      REG_SZ,
      reinterpret_cast<const BYTE*>(id.c_str()),
      static_cast<DWORD>(id.size() + 1));
  RegCloseKey(key);
  return status == ERROR_SUCCESS;
}

bool ClientId::Clear() {
  HKEY key = nullptr;
  if (RegOpenKeyExA(HKEY_CURRENT_USER, kRegistryKey, 0, KEY_WRITE, &key) != ERROR_SUCCESS) {
    return true;
  }
  RegDeleteValueA(key, kValueName);
  RegCloseKey(key);
  return true;
}
#else
std::string ClientId::GetStored() {
  return "";
}

bool ClientId::Store(const std::string&) {
  return false;
}

bool ClientId::Clear() {
  return true;
}
#endif

std::string ClientId::GetOrCreate() {
  std::string id = GetStored();
  if (!id.empty()) {
    return id;
  }

  id = Generate();
  Store(id);
  return id;
}
