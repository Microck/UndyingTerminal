#include "TcpClient.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cerrno>

namespace {
std::string StripBrackets(const std::string& host) {
  if (host.size() >= 2 && host.front() == '[' && host.back() == ']') {
    return host.substr(1, host.size() - 2);
  }
  return host;
}
}

TcpClient::TcpClient() = default;

TcpClient::~TcpClient() {
  Close();
}

bool TcpClient::Connect(const std::string& host, uint16_t port) {
  Close();
  last_error_ = 0;

  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  addrinfo* result = nullptr;
  const std::string host_name = StripBrackets(host);
  const std::string port_str = std::to_string(port);
  const int rc = getaddrinfo(host_name.c_str(), port_str.c_str(), &hints, &result);
  if (rc != 0) {
    last_error_ = rc;
    return false;
  }

  SOCKET sock = INVALID_SOCKET;
  int last_connect_error = 0;
  for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
    sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (sock == INVALID_SOCKET) {
      last_connect_error = WSAGetLastError();
      continue;
    }
    if (connect(sock, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen)) == 0) {
      break;
    }
    last_connect_error = WSAGetLastError();
    closesocket(sock);
    sock = INVALID_SOCKET;
  }
  freeaddrinfo(result);

  if (sock == INVALID_SOCKET) {
    last_error_ = last_connect_error;
    return false;
  }

  socket_ = static_cast<unsigned long long>(sock);
  return true;
}

bool TcpClient::Send(const std::string& data) {
  if (!IsConnected()) {
    last_error_ = WSAENOTCONN;
    return false;
  }
  const int sent = send(static_cast<SOCKET>(socket_), data.data(), static_cast<int>(data.size()), 0);
  if (sent == static_cast<int>(data.size())) {
    return true;
  }
  last_error_ = sent == SOCKET_ERROR ? WSAGetLastError() : 0;
  return false;
}

bool TcpClient::Receive(std::string* out) {
  if (!IsConnected() || !out) {
    last_error_ = WSAENOTCONN;
    return false;
  }
  char buffer[256] = {};
  const int received = recv(static_cast<SOCKET>(socket_), buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    last_error_ = received == 0 ? 0 : WSAGetLastError();
    return false;
  }
  out->assign(buffer, buffer + received);
  return true;
}

void TcpClient::Close() {
  if (socket_ != static_cast<unsigned long long>(INVALID_SOCKET)) {
    closesocket(static_cast<SOCKET>(socket_));
    socket_ = static_cast<unsigned long long>(INVALID_SOCKET);
  }
}

bool TcpClient::IsConnected() const {
  return socket_ != static_cast<unsigned long long>(INVALID_SOCKET);
}
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>

namespace {
std::string StripBrackets(const std::string& host) {
  if (host.size() >= 2 && host.front() == '[' && host.back() == ']') {
    return host.substr(1, host.size() - 2);
  }
  return host;
}
}

TcpClient::TcpClient() = default;

TcpClient::~TcpClient() {
  Close();
}

bool TcpClient::Connect(const std::string& host, uint16_t port) {
  Close();
  last_error_ = 0;

  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  addrinfo* result = nullptr;
  const std::string host_name = StripBrackets(host);
  const std::string port_str = std::to_string(port);
  const int rc = getaddrinfo(host_name.c_str(), port_str.c_str(), &hints, &result);
  if (rc != 0) {
    last_error_ = rc;
    return false;
  }

  int sock = -1;
  int last_connect_error = 0;
  for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
    sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (sock < 0) {
      last_connect_error = errno;
      continue;
    }
    if (connect(sock, reinterpret_cast<sockaddr*>(ptr->ai_addr), ptr->ai_addrlen) == 0) {
      break;
    }
    last_connect_error = errno;
    close(sock);
    sock = -1;
  }
  freeaddrinfo(result);

  if (sock < 0) {
    last_error_ = last_connect_error;
    return false;
  }

  socket_ = sock;
  return true;
}

bool TcpClient::Send(const std::string& data) {
  if (!IsConnected()) {
    last_error_ = ENOTCONN;
    return false;
  }
  const ssize_t sent = send(socket_, data.data(), data.size(), 0);
  if (sent == static_cast<ssize_t>(data.size())) {
    return true;
  }
  last_error_ = sent < 0 ? errno : 0;
  return false;
}

bool TcpClient::Receive(std::string* out) {
  if (!IsConnected() || !out) {
    last_error_ = ENOTCONN;
    return false;
  }
  char buffer[256] = {};
  const ssize_t received = recv(socket_, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    last_error_ = received == 0 ? 0 : errno;
    return false;
  }
  out->assign(buffer, buffer + received);
  return true;
}

void TcpClient::Close() {
  if (socket_ >= 0) {
    close(socket_);
    socket_ = -1;
  }
}

bool TcpClient::IsConnected() const {
  return socket_ >= 0;
}
#endif
