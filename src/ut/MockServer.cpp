#include "MockServer.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>

MockServer::MockServer() = default;

MockServer::~MockServer() {
  Stop();
}

bool MockServer::Start(uint16_t* port_out) {
  Stop();

  WSADATA wsa{};
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    std::cerr << "WSAStartup failed: " << WSAGetLastError() << "\n";
    return false;
  }

  SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == INVALID_SOCKET) {
    std::cerr << "socket failed: " << WSAGetLastError() << "\n";
    return false;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0;

  if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
    std::cerr << "bind failed: " << WSAGetLastError() << "\n";
    closesocket(sock);
    return false;
  }

  if (listen(sock, 1) == SOCKET_ERROR) {
    std::cerr << "listen failed: " << WSAGetLastError() << "\n";
    closesocket(sock);
    return false;
  }

  sockaddr_in actual{};
  int actual_len = sizeof(actual);
  if (getsockname(sock, reinterpret_cast<sockaddr*>(&actual), &actual_len) == SOCKET_ERROR) {
    std::cerr << "getsockname failed: " << WSAGetLastError() << "\n";
    closesocket(sock);
    return false;
  }

  if (port_out) {
    *port_out = ntohs(actual.sin_port);
  }

  listen_socket_ = static_cast<unsigned long long>(sock);
  running_ = true;
  accept_thread_ = std::thread(&MockServer::AcceptLoop, this);
  return true;
}

void MockServer::Stop() {
  if (listen_socket_ != static_cast<unsigned long long>(INVALID_SOCKET)) {
    closesocket(static_cast<SOCKET>(listen_socket_));
    listen_socket_ = static_cast<unsigned long long>(INVALID_SOCKET);
  }
  if (client_socket_ != static_cast<unsigned long long>(INVALID_SOCKET)) {
    closesocket(static_cast<SOCKET>(client_socket_));
    client_socket_ = static_cast<unsigned long long>(INVALID_SOCKET);
  }
  running_ = false;
  if (accept_thread_.joinable()) {
    accept_thread_.join();
  }
}

void MockServer::AcceptLoop() {
  SOCKET sock = static_cast<SOCKET>(listen_socket_);
  if (sock == INVALID_SOCKET) {
    return;
  }

  SOCKET client = accept(sock, nullptr, nullptr);
  if (client != INVALID_SOCKET) {
    client_socket_ = static_cast<unsigned long long>(client);
    closesocket(client);
    client_socket_ = static_cast<unsigned long long>(INVALID_SOCKET);
  }
}
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

MockServer::MockServer() = default;

MockServer::~MockServer() {
  Stop();
}

bool MockServer::Start(uint16_t* port_out) {
  Stop();

  const int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    return false;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0;

  if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    close(sock);
    return false;
  }

  if (listen(sock, 1) < 0) {
    close(sock);
    return false;
  }

  sockaddr_in actual{};
  socklen_t actual_len = sizeof(actual);
  if (getsockname(sock, reinterpret_cast<sockaddr*>(&actual), &actual_len) < 0) {
    close(sock);
    return false;
  }

  if (port_out) {
    *port_out = ntohs(actual.sin_port);
  }

  listen_socket_ = sock;
  running_ = true;
  accept_thread_ = std::thread(&MockServer::AcceptLoop, this);
  return true;
}

void MockServer::Stop() {
  if (listen_socket_ >= 0) {
    close(listen_socket_);
    listen_socket_ = -1;
  }
  if (client_socket_ >= 0) {
    close(client_socket_);
    client_socket_ = -1;
  }
  running_ = false;
  if (accept_thread_.joinable()) {
    accept_thread_.join();
  }
}

void MockServer::AcceptLoop() {
  const int sock = listen_socket_;
  if (sock < 0) {
    return;
  }

  const int client = accept(sock, nullptr, nullptr);
  if (client >= 0) {
    client_socket_ = client;
    close(client);
    client_socket_ = -1;
  }
}
#endif
