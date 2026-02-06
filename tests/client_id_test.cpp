#include <iostream>

#include "ClientId.hpp"

int main() {
  std::string id1 = ClientId::Generate();
  std::string id2 = ClientId::Generate();

  if (id1.empty() || id2.empty()) {
    std::cerr << "Generate returned empty\n";
    return 1;
  }
  if (id1 == id2) {
    std::cerr << "Generate returned duplicate\n";
    return 1;
  }
  if (id1.find('-') == std::string::npos) {
    std::cerr << "Invalid format\n";
    return 1;
  }

  ClientId::Clear();
  std::string stored = ClientId::GetStored();
  if (!stored.empty()) {
    std::cerr << "Clear failed\n";
    return 1;
  }

  ClientId::Store("test-id-123");
  stored = ClientId::GetStored();
  if (stored != "test-id-123") {
    std::cerr << "Store/Get failed: " << stored << "\n";
    return 1;
  }

  std::string got = ClientId::GetOrCreate();
  if (got != "test-id-123") {
    std::cerr << "GetOrCreate should return existing\n";
    return 1;
  }

  ClientId::Clear();
  got = ClientId::GetOrCreate();
  if (got.empty()) {
    std::cerr << "GetOrCreate should generate new\n";
    return 1;
  }

  ClientId::Clear();

  std::cout << "Client ID test passed\n";
  return 0;
}
