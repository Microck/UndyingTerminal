#include "NamedPipeClient.hpp"
#include "Server.hpp"
#include "TcpClient.hpp"
#include "WinsockContext.hpp"

#include <iostream>

int main() {
#ifdef _WIN32
  try {
    WinsockContext winsock;
    (void)winsock;
  } catch (const std::exception& ex) {
    std::cerr << "Winsock init failed: " << ex.what() << "\n";
    return 1;
  }

  Server server;
  if (!server.Start(0)) {
    std::cerr << "Server failed to start\n";
    return 1;
  }

  NamedPipeClient pipe;
  if (!pipe.Connect(L"\\\\.\\pipe\\undying-terminal")) {
    std::cerr << "Named pipe connect failed\n";
    return 1;
  }
  if (!pipe.Send("REGISTER test-client\n")) {
    std::cerr << "Named pipe register failed\n";
    return 1;
  }
  std::string pipe_reply;
  if (!pipe.Receive(&pipe_reply) || pipe_reply.find("OK") == std::string::npos) {
    std::cerr << "Named pipe reply failed\n";
    return 1;
  }

  for (int attempt = 0; attempt < 2; ++attempt) {
    TcpClient client;
    if (!client.Connect("127.0.0.1", server.port())) {
      std::cerr << "TCP connect failed\n";
      return 1;
    }
    if (!client.Send("CONNECT test-client\n")) {
      std::cerr << "TCP send failed\n";
      return 1;
    }
    std::string reply;
    client.Receive(&reply);
    if (reply.find("CONNECTED") == std::string::npos) {
      std::cerr << "Unexpected reply\n";
      return 1;
    }
  }

  server.Stop();
  return 0;
#else
  return 0;
#endif
}
