#include "TcpClient.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cerrno>

TcpClient::TcpClient() = default;

TcpClient::~TcpClient() {
  Close();
}

bool TcpClient::Connect(const std::string& host, uint16_t port) {
  Close();
  last_error_ = 0;

  SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    last_error_ = WSAGetLastError();
    return false;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (InetPtonA(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
    last_error_ = WSAGetLastError();
    closesocket(sock);
    return false;
  }

  if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
    last_error_ = WSAGetLastError();
    closesocket(sock);
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
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>

TcpClient::TcpClient() = default;

TcpClient::~TcpClient() {
  Close();
}

bool TcpClient::Connect(const std::string& host, uint16_t port) {
  Close();
  last_error_ = 0;

  const int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    last_error_ = errno;
    return false;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
    last_error_ = errno;
    close(sock);
    return false;
  }

  if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    last_error_ = errno;
    close(sock);
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
