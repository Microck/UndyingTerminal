#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include <atomic>

#include "Config.hpp"
#include "CryptoUtils.hpp"
#include "FirewallRules.hpp"
#include "Server.hpp"
#include "Verbose.hpp"
#include "WindowsService.hpp"
#include "WinsockContext.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
bool RunSelfTest() {
  SetVerbose(true);
  try {
    WinsockContext winsock;
    (void)winsock;
  } catch (const std::exception& ex) {
    if (IsVerbose()) {
      std::cerr << "Winsock init failed: " << ex.what() << "\n";
    }
    return false;
  }

  Server server;
  if (!server.Start(0)) {
    if (IsVerbose()) {
      std::cerr << "Server failed to start\n";
    }
    return false;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  server.Stop();
  return true;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc > 1 && std::string(argv[1]) == "--self-test") {
    return RunSelfTest() ? 0 : 1;
  }

  if (argc > 1 && std::string(argv[1]) == "--add-firewall") {
    Config config;
    config.Load();
    return FirewallRules::EnsureRule(config.port) ? 0 : 1;
  }

  if (argc > 1 && std::string(argv[1]) == "--service") {
    Server server;
    return WindowsService::RunAsService(&server);
  }

  std::cout << "Undying Terminal server starting..." << std::endl;
  Config config;
  config.Load();
  SetVerbose(config.verbose);
  Server server;
#ifdef UNDYING_TERMINAL_REQUIRE_DEPS
  if (!config.shared_key_hex.empty()) {
    std::array<unsigned char, 32> key{};
    if (CryptoUtils::DecodeHexKey(config.shared_key_hex, &key)) {
      server.SetSharedKey(key);
    }
  }
#endif
  if (!server.Start(static_cast<uint16_t>(config.port), config.bind_ip)) {
    if (IsVerbose()) {
      std::cerr << "Failed to start server" << std::endl;
    }
    return 1;
  }

  std::atomic<bool> running{true};
#ifdef _WIN32
  static std::atomic<bool>* running_ptr = nullptr;
  running_ptr = &running;
  SetConsoleCtrlHandler(
      [](DWORD signal) -> BOOL {
        if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT || signal == CTRL_BREAK_EVENT) {
          if (running_ptr) {
            running_ptr->store(false);
          }
          return TRUE;
        }
        return FALSE;
      },
      TRUE);
#endif

  std::cout << "Press Ctrl+C to stop" << std::endl;
  while (running.load()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  server.Stop();
  return 0;
}
