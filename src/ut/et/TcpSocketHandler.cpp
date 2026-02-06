#include "TcpSocketHandler.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <cstring>
#include <string>

namespace ut {
TcpSocketHandler::TcpSocketHandler() {
  EnsureWinsock();
}

TcpSocketHandler::~TcpSocketHandler() = default;

bool TcpSocketHandler::EnsureWinsock() {
#ifdef _WIN32
  if (winsock_ready_) {
    return true;
  }
  WSADATA wsa{};
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    return false;
  }
  winsock_ready_ = true;
#endif
  return true;
}

bool TcpSocketHandler::HasData(SocketHandle socket) {
#ifdef _WIN32
  if (socket == kInvalidSocket) {
    return false;
  }
  fd_set read_set;
  FD_ZERO(&read_set);
  FD_SET(static_cast<SOCKET>(socket), &read_set);
  timeval timeout{};
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  const int result = select(0, &read_set, nullptr, nullptr, &timeout);
  return result > 0 && FD_ISSET(static_cast<SOCKET>(socket), &read_set);
#else
  (void)socket;
  return false;
#endif
}

int TcpSocketHandler::Read(SocketHandle socket, void* buf, size_t count) {
#ifdef _WIN32
  if (socket == kInvalidSocket) {
    return -1;
  }
  return recv(static_cast<SOCKET>(socket), reinterpret_cast<char*>(buf), static_cast<int>(count), 0);
#else
  (void)socket;
  (void)buf;
  (void)count;
  return -1;
#endif
}

int TcpSocketHandler::Write(SocketHandle socket, const void* buf, size_t count) {
#ifdef _WIN32
  if (socket == kInvalidSocket) {
    return -1;
  }
  return send(static_cast<SOCKET>(socket), reinterpret_cast<const char*>(buf), static_cast<int>(count), 0);
#else
  (void)socket;
  (void)buf;
  (void)count;
  return -1;
#endif
}

void TcpSocketHandler::Close(SocketHandle socket) {
#ifdef _WIN32
  if (socket != kInvalidSocket) {
    closesocket(static_cast<SOCKET>(socket));
  }
#else
  (void)socket;
#endif
}

SocketHandle TcpSocketHandler::Connect(const std::string& host, int port) {
#ifdef _WIN32
  if (!EnsureWinsock()) {
    return kInvalidSocket;
  }
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  addrinfo* result = nullptr;
  const std::string port_str = std::to_string(port);
  if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result) != 0) {
    return kInvalidSocket;
  }

  SocketHandle sock_handle = kInvalidSocket;
  for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
    SOCKET sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (sock == INVALID_SOCKET) {
      continue;
    }
    if (connect(sock, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen)) == 0) {
      sock_handle = static_cast<SocketHandle>(sock);
      break;
    }
    closesocket(sock);
  }
  freeaddrinfo(result);
  return sock_handle;
#else
  (void)host;
  (void)port;
  return kInvalidSocket;
#endif
}

SocketHandle TcpSocketHandler::Listen(const std::string& bind_ip, int port) {
#ifdef _WIN32
  if (!EnsureWinsock()) {
    return kInvalidSocket;
  }
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  addrinfo* result = nullptr;
  const std::string port_str = std::to_string(port);
  const char* host = bind_ip.empty() ? nullptr : bind_ip.c_str();
  if (getaddrinfo(host, port_str.c_str(), &hints, &result) != 0) {
    return kInvalidSocket;
  }

  SocketHandle listen_handle = kInvalidSocket;
  for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
    SOCKET sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (sock == INVALID_SOCKET) {
      continue;
    }
    if (ptr->ai_family == AF_INET6) {
      DWORD v6_only = 0;
      setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&v6_only), sizeof(v6_only));
    }
    if (bind(sock, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen)) == 0 &&
        listen(sock, SOMAXCONN) == 0) {
      listen_handle = static_cast<SocketHandle>(sock);
      break;
    }
    closesocket(sock);
  }
  freeaddrinfo(result);
  return listen_handle;
#else
  (void)bind_ip;
  (void)port;
  return kInvalidSocket;
#endif
}

SocketHandle TcpSocketHandler::Accept(SocketHandle listen_socket) {
#ifdef _WIN32
  if (listen_socket == kInvalidSocket) {
    return kInvalidSocket;
  }
  SOCKET client = accept(static_cast<SOCKET>(listen_socket), nullptr, nullptr);
  if (client == INVALID_SOCKET) {
    return kInvalidSocket;
  }
  return static_cast<SocketHandle>(client);
#else
  (void)listen_socket;
  return kInvalidSocket;
#endif
}

uint16_t TcpSocketHandler::GetBoundPort(SocketHandle socket) {
#ifdef _WIN32
  if (socket == kInvalidSocket) {
    return 0;
  }
  sockaddr_storage addr{};
  int len = sizeof(addr);
  if (getsockname(static_cast<SOCKET>(socket), reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
    return 0;
  }
  if (addr.ss_family == AF_INET) {
    auto* in = reinterpret_cast<sockaddr_in*>(&addr);
    return ntohs(in->sin_port);
  }
  if (addr.ss_family == AF_INET6) {
    auto* in6 = reinterpret_cast<sockaddr_in6*>(&addr);
    return ntohs(in6->sin6_port);
  }
#endif
  return 0;
}
}
