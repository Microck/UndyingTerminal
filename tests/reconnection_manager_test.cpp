#include <iostream>

#include "ReconnectionManager.hpp"

int main() {
  ReconnectionManager mgr;
  mgr.SetTarget("localhost", 2022, "test-client");

  int attempts = 0;
  bool result = mgr.AttemptReconnect([&](const std::string&, int) {
    ++attempts;
    return attempts >= 3;
  });

  if (!result) {
    std::cerr << "Should have succeeded\n";
    return 1;
  }

  if (attempts != 3) {
    std::cerr << "Wrong attempt count: " << attempts << "\n";
    return 1;
  }

  ReconnectionManager mgr2;
  mgr2.SetTarget("localhost", 2022, "test-client");

  result = mgr2.AttemptReconnect([](const std::string&, int) {
    return false;
  });

  if (result) {
    std::cerr << "Should have failed\n";
    return 1;
  }

  std::cout << "Reconnection manager test passed\n";
  return 0;
}
