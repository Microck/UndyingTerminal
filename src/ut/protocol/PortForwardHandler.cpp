#include "PortForwardHandler.hpp"

#include <cstdlib>
#include <iostream>

namespace ut {
namespace {
bool DebugTunnel() {
  return std::getenv("UT_DEBUG_HANDSHAKE") != nullptr;
}
}
PortForwardHandler::PortForwardHandler(std::shared_ptr<TcpSocketHandler> socket_handler, bool server_side)
    : socket_handler_(std::move(socket_handler)), server_side_(server_side) {}

void PortForwardHandler::AddForwardRequest(const ut::PortForwardSourceRequest& request) {
  if (server_side_) {
    return;
  }
  if (!request.has_source() || !request.has_destination()) {
    return;
  }
  if (DebugTunnel()) {
    std::cerr << "[tunnel] add_forward source=" << request.source().name()
              << ":" << request.source().port()
              << " dest=" << request.destination().name()
              << ":" << request.destination().port() << "\n";
  }
  Listener listener;
  listener.destination = request.destination();
  const std::string bind_name = request.source().name().empty() ? "localhost" : request.source().name();
  listener.listen_socket = socket_handler_->Listen(bind_name, request.source().port());
  if (listener.listen_socket == kInvalidSocket) {
    return;
  }
  listeners_.push_back(listener);
}

void PortForwardHandler::Update(const std::function<void(const Packet&)>& send_packet) {
  if (server_side_) {
    HandleServerData(send_packet);
  } else {
    HandleClientData(send_packet);
  }
}

void PortForwardHandler::HandleClientData(const std::function<void(const Packet&)>& send_packet) {
  for (auto& listener : listeners_) {
    if (!socket_handler_->HasData(listener.listen_socket)) {
      continue;
    }
    SocketHandle client_socket = socket_handler_->Accept(listener.listen_socket);
    if (client_socket == kInvalidSocket) {
      continue;
    }
    const int client_fd = next_client_fd_++;
    pending_clients_[client_fd] = client_socket;
    if (DebugTunnel()) {
      std::cerr << "[tunnel] accept client_fd=" << client_fd
                << " dest=" << listener.destination.name()
                << ":" << listener.destination.port() << "\n";
    }

    ut::PortForwardDestinationRequest req;
    *req.mutable_destination() = listener.destination;
    req.set_fd(client_fd);
    std::string payload;
    if (!req.SerializeToString(&payload)) {
      continue;
    }
    send_packet(Packet(static_cast<uint8_t>(ut::PORT_FORWARD_DESTINATION_REQUEST), payload));
  }

  for (auto& entry : active_sockets_) {
    const int socket_id = entry.first;
    SocketHandle local_socket = entry.second;
    if (!socket_handler_->HasData(local_socket)) {
      continue;
    }
    char buffer[4096] = {};
    const int rc = socket_handler_->Read(local_socket, buffer, sizeof(buffer));
    if (rc <= 0) {
      ut::PortForwardData data;
      data.set_socketid(socket_id);
      data.set_sourcetodestination(true);
      data.set_closed(true);
      std::string payload;
      if (data.SerializeToString(&payload)) {
        send_packet(Packet(static_cast<uint8_t>(ut::PORT_FORWARD_DATA), payload));
      }
      socket_handler_->Close(local_socket);
      continue;
    }
    ut::PortForwardData data;
    data.set_socketid(socket_id);
    data.set_sourcetodestination(true);
    data.set_buffer(std::string(buffer, buffer + rc));
    std::string payload;
    if (data.SerializeToString(&payload)) {
      if (DebugTunnel()) {
        std::cerr << "[tunnel] client_data socket_id=" << socket_id
                  << " bytes=" << rc << "\n";
      }
      send_packet(Packet(static_cast<uint8_t>(ut::PORT_FORWARD_DATA), payload));
    }
  }
}

void PortForwardHandler::HandleServerData(const std::function<void(const Packet&)>& send_packet) {
  for (auto& entry : active_sockets_) {
    const int socket_id = entry.first;
    SocketHandle remote_socket = entry.second;
    if (!socket_handler_->HasData(remote_socket)) {
      continue;
    }
    char buffer[4096] = {};
    const int rc = socket_handler_->Read(remote_socket, buffer, sizeof(buffer));
    if (rc <= 0) {
      ut::PortForwardData data;
      data.set_socketid(socket_id);
      data.set_sourcetodestination(false);
      data.set_closed(true);
      std::string payload;
      if (data.SerializeToString(&payload)) {
        send_packet(Packet(static_cast<uint8_t>(ut::PORT_FORWARD_DATA), payload));
      }
      socket_handler_->Close(remote_socket);
      continue;
    }
    ut::PortForwardData data;
    data.set_socketid(socket_id);
    data.set_sourcetodestination(false);
    data.set_buffer(std::string(buffer, buffer + rc));
    std::string payload;
    if (data.SerializeToString(&payload)) {
      if (DebugTunnel()) {
        std::cerr << "[tunnel] server_data socket_id=" << socket_id
                  << " bytes=" << rc << "\n";
      }
      send_packet(Packet(static_cast<uint8_t>(ut::PORT_FORWARD_DATA), payload));
    }
  }
}

void PortForwardHandler::HandlePacket(const Packet& packet, const std::function<void(const Packet&)>& send_packet) {
  if (packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DESTINATION_RESPONSE)) {
    ut::PortForwardDestinationResponse response;
    if (!response.ParseFromString(packet.payload())) {
      return;
    }
    if (!response.has_socketid() || !response.has_clientfd()) {
      return;
    }
    if (DebugTunnel()) {
      std::cerr << "[tunnel] dest_response client_fd=" << response.clientfd()
                << " socket_id=" << response.socketid()
                << " error=" << response.error() << "\n";
    }
    auto it = pending_clients_.find(response.clientfd());
    if (it == pending_clients_.end()) {
      return;
    }
    if (!response.error().empty()) {
      socket_handler_->Close(it->second);
      pending_clients_.erase(it);
      return;
    }
    active_sockets_[response.socketid()] = it->second;
    pending_clients_.erase(it);
    return;
  }

  if (packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DESTINATION_REQUEST) && server_side_) {
    ut::PortForwardDestinationRequest request;
    if (!request.ParseFromString(packet.payload())) {
      return;
    }
    if (!request.has_destination() || !request.has_fd()) {
      return;
    }
    if (DebugTunnel()) {
      std::cerr << "[tunnel] dest_request fd=" << request.fd()
                << " dest=" << request.destination().name()
                << ":" << request.destination().port() << "\n";
    }
    SocketHandle remote_socket = socket_handler_->Connect(request.destination().name(), request.destination().port());
    ut::PortForwardDestinationResponse response;
    response.set_clientfd(request.fd());
    if (remote_socket == kInvalidSocket) {
      response.set_error("connect failed");
    } else {
      int socket_id = next_socket_id_++;
      active_sockets_[socket_id] = remote_socket;
      response.set_socketid(socket_id);
    }
    std::string payload;
    if (response.SerializeToString(&payload)) {
      send_packet(Packet(static_cast<uint8_t>(ut::PORT_FORWARD_DESTINATION_RESPONSE), payload));
    }
    return;
  }

  if (packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DATA)) {
    ut::PortForwardData data;
    if (!data.ParseFromString(packet.payload())) {
      return;
    }
    if (!data.has_socketid()) {
      return;
    }
    auto it = active_sockets_.find(data.socketid());
    if (it == active_sockets_.end()) {
      return;
    }
    SocketHandle target = it->second;
    if (data.closed()) {
      socket_handler_->Close(target);
      active_sockets_.erase(it);
      return;
    }
    if (!data.buffer().empty()) {
      if (DebugTunnel()) {
        std::cerr << "[tunnel] write_data socket_id=" << data.socketid()
                  << " bytes=" << data.buffer().size()
                  << " src_to_dst=" << data.sourcetodestination() << "\n";
      }
      socket_handler_->Write(target, data.buffer().data(), data.buffer().size());
    }
  }
}
}
