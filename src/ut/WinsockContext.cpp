#include "WinsockContext.hpp"

#ifdef _WIN32
#include <stdexcept>
#include <string>

WinsockContext::WinsockContext() {
  WSADATA wsa_data{};
  const int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (result != 0) {
    throw std::runtime_error("WSAStartup failed: " + std::to_string(result));
  }

  if (LOBYTE(wsa_data.wVersion) != 2 || HIBYTE(wsa_data.wVersion) != 2) {
    WSACleanup();
    throw std::runtime_error("Winsock 2.2 not available");
  }

  initialized_ = true;
}

WinsockContext::~WinsockContext() {
  if (initialized_) {
    WSACleanup();
  }
}
#else
WinsockContext::WinsockContext() = default;
WinsockContext::~WinsockContext() = default;
#endif
