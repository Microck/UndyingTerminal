#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ETerminal.pb.h"
#include "Packet.hpp"
#include "TcpSocketHandler.hpp"

namespace ut {
class PortForwardHandler {
 public:
  PortForwardHandler(std::shared_ptr<TcpSocketHandler> socket_handler, bool server_side);

  void AddForwardRequest(const et::PortForwardSourceRequest& request);
  void Update(const std::function<void(const Packet&)>& send_packet);
  void HandlePacket(const Packet& packet, const std::function<void(const Packet&)>& send_packet);

 private:
  struct Listener {
    SocketHandle listen_socket = kInvalidSocket;
    et::SocketEndpoint destination;
  };

  void HandleClientData(const std::function<void(const Packet&)>& send_packet);
  void HandleServerData(const std::function<void(const Packet&)>& send_packet);

  std::shared_ptr<TcpSocketHandler> socket_handler_;
  bool server_side_ = false;
  int next_client_fd_ = 1;
  int next_socket_id_ = 1;

  std::vector<Listener> listeners_;
  std::unordered_map<int, SocketHandle> pending_clients_;
  std::unordered_map<int, SocketHandle> active_sockets_;
};
}
