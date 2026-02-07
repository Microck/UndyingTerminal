#include "TcpListener.hpp"

#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>

#include "ClientRegistry.hpp"
#include "Verbose.hpp"
#include "protocol/PipeSocketHandler.hpp"
#include "protocol/PortForwardHandler.hpp"
#include "protocol/ServerClientConnection.hpp"
#include "UtConstants.hpp"
#include "UT.pb.h"
#include "UTerminal.pb.h"

namespace {
bool DebugHandshake() {
  return std::getenv("UT_DEBUG_HANDSHAKE") != nullptr;
}
bool SendTermInit(ut::PipeSocketHandler& pipe_handler, ut::SocketHandle pipe_handle) {
  ut::TermInit init;
  std::string payload;
  if (!init.SerializeToString(&payload)) {
    return false;
  }
  ut::Packet packet(static_cast<uint8_t>(ut::TERMINAL_INIT), payload);
  pipe_handler.WritePacket(pipe_handle, packet);
  return true;
}

bool SendJumpInit(ut::PipeSocketHandler& pipe_handler,
                  ut::SocketHandle pipe_handle,
                  const ut::InitialPayload& payload_msg) {
  std::string payload;
  if (!payload_msg.SerializeToString(&payload)) {
    return false;
  }
  ut::Packet packet(static_cast<uint8_t>(ut::JUMPHOST_INIT), payload);
  pipe_handler.WritePacket(pipe_handle, packet);
  return true;
}

}

TcpListener::TcpListener() = default;

TcpListener::~TcpListener() {
  Stop();
}

bool TcpListener::Start(uint16_t port, const std::string& bind_ip, ClientRegistry* registry) {
  Stop();
  registry_ = registry;
  socket_handler_ = std::make_shared<ut::TcpSocketHandler>();
  listen_socket_ = socket_handler_->Listen(bind_ip, port);
  if (listen_socket_ == ut::kInvalidSocket) {
    if (IsVerbose()) {
      std::cerr << "TcpListener failed to bind\n";
    }
    return false;
  }
  port_ = socket_handler_->GetBoundPort(listen_socket_);
  running_ = true;
  accept_thread_ = std::thread(&TcpListener::AcceptLoop, this);
  return true;
}

void TcpListener::Stop() {
  running_ = false;
  if (listen_socket_ != ut::kInvalidSocket) {
    socket_handler_->Close(listen_socket_);
    listen_socket_ = ut::kInvalidSocket;
  }
  port_ = 0;
  if (accept_thread_.joinable()) {
    accept_thread_.join();
  }
}

void TcpListener::SetSharedKey(const std::array<unsigned char, 32>& key) {
  shared_key_ = key;
  encryption_enabled_ = true;
}

void TcpListener::AcceptLoop() {
  while (running_) {
    if (!socket_handler_->HasData(listen_socket_)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      continue;
    }
    ut::SocketHandle client = socket_handler_->Accept(listen_socket_);
    if (client == ut::kInvalidSocket) {
      continue;
    }
    std::thread(&TcpListener::HandleClient, this, client).detach();
  }
}

void TcpListener::HandleClient(ut::SocketHandle client) {
  if (!registry_) {
    socket_handler_->Close(client);
    return;
  }

  ut::ConnectRequest request;
  try {
    request = socket_handler_->ReadProto<ut::ConnectRequest>(client, true);
  } catch (...) {
    socket_handler_->Close(client);
    return;
  }
  if (DebugHandshake()) {
    std::cerr << "[handshake] connect_request client_id_len=" << request.clientid().size()
              << " version=" << request.version() << "\n";
  }

  ut::ConnectResponse response;
  if (request.version() != ut::kProtocolVersion) {
    response.set_status(ut::MISMATCHED_PROTOCOL);
    response.set_error("protocol mismatch");
    socket_handler_->WriteProto(client, response, true);
    socket_handler_->Close(client);
    return;
  }

  const std::string client_id = request.clientid();
  if (!registry_->HasSession(client_id)) {
    response.set_status(ut::INVALID_KEY);
    response.set_error("unknown client id");
    socket_handler_->WriteProto(client, response, true);
    socket_handler_->Close(client);
    return;
  }

  std::string passkey = registry_->LookupPasskey(client_id);
  if (DebugHandshake()) {
    std::cerr << "[handshake] passkey_len=" << passkey.size()
              << " has_underscore=" << (passkey.find('_') != std::string::npos) << "\n";
  }
  if (passkey.empty()) {
    response.set_status(ut::INVALID_KEY);
    response.set_error("missing key");
    socket_handler_->WriteProto(client, response, true);
    socket_handler_->Close(client);
    return;
  }

  auto existing = registry_->LookupConnection(client_id);
  if (existing && existing->socket() == ut::kInvalidSocket) {
    response.set_status(ut::RETURNING_CLIENT);
    socket_handler_->WriteProto(client, response, true);
    if (!existing->Recover(client)) {
      socket_handler_->Close(client);
    }
    return;
  }

  response.set_status(ut::NEW_CLIENT);
  socket_handler_->WriteProto(client, response, true);

  auto connection = std::make_shared<ut::ServerClientConnection>(socket_handler_, client_id, passkey, client);
  registry_->StoreConnection(client_id, connection);
  registry_->MarkActive(client_id, true);

  ut::Packet init_packet;
  try {
    if (!connection->ReadPacket(&init_packet) ||
        init_packet.header() != static_cast<uint8_t>(ut::INITIAL_PAYLOAD)) {
      if (DebugHandshake()) {
        std::cerr << "[handshake] initial_payload_read_failed header="
                  << static_cast<int>(init_packet.header()) << "\n";
      }
      connection->CloseSocket();
      registry_->MarkActive(client_id, false);
      return;
    }
  } catch (const std::exception& ex) {
    if (DebugHandshake()) {
      std::cerr << "[handshake] initial_payload_read_exception: " << ex.what() << "\n";
    }
    connection->CloseSocket();
    registry_->MarkActive(client_id, false);
    return;
  }
  ut::InitialPayload initial_payload;
  if (!initial_payload.ParseFromString(init_packet.payload())) {
    if (DebugHandshake()) {
      std::cerr << "[handshake] initial_payload_parse_failed size="
                << init_packet.payload().size() << "\n";
    }
    connection->CloseSocket();
    registry_->MarkActive(client_id, false);
    return;
  }

  ut::InitialResponse initial_response;
  std::string response_payload;
  initial_response.SerializeToString(&response_payload);
  if (DebugHandshake()) {
    std::cerr << "[handshake] sending_initial_response size=" << response_payload.size() << "\n";
  }
  connection->WritePacket(ut::Packet(static_cast<uint8_t>(ut::INITIAL_RESPONSE), response_payload));

  ut::PipeSocketHandler pipe_handler;
  ut::PortForwardHandler forward_handler(socket_handler_, true);
  ut::PortForwardHandler reverse_handler(socket_handler_, false);
  ut::SocketHandle pipe = registry_->LookupTerminal(client_id);
  if (pipe == ut::kInvalidSocket) {
    connection->CloseSocket();
    registry_->MarkActive(client_id, false);
    return;
  }
  const bool jump_mode = initial_payload.jumphost();
  for (const auto& reverse_tunnel : initial_payload.reversetunnels()) {
    reverse_handler.AddForwardRequest(reverse_tunnel);
  }
  const bool init_sent = jump_mode ? SendJumpInit(pipe_handler, pipe, initial_payload)
                                    : SendTermInit(pipe_handler, pipe);
  if (!init_sent) {
    connection->CloseSocket();
    registry_->MarkActive(client_id, false);
    return;
  }

  while (running_) {
    if (!pipe_handler.IsConnected(pipe)) {
      if (DebugHandshake()) {
        std::cerr << "[handshake] term pipe disconnected\n";
      }
      break;
    }
    bool did_work = false;
    if (connection->reader() && connection->reader()->HasData()) {
      ut::Packet packet;
      bool read_ok = false;
      try {
        read_ok = connection->ReadPacket(&packet);
      } catch (const std::exception& ex) {
        if (DebugHandshake()) {
          std::cerr << "[handshake] read_packet_exception: " << ex.what() << "\n";
        }
        break;
      }
      if (!read_ok) {
        if (DebugHandshake()) {
          std::cerr << "[handshake] read_packet_failed\n";
        }
      } else {
        if (packet.header() == static_cast<uint8_t>(ut::TERMINAL_BUFFER) ||
            packet.header() == static_cast<uint8_t>(ut::TERMINAL_INFO)) {
          if (DebugHandshake()) {
            std::cerr << "[handshake] term client_to_pipe header="
                      << static_cast<int>(packet.header())
                      << " bytes=" << packet.payload().size()
                      << " jump=" << (jump_mode ? 1 : 0) << "\n";
          }
          pipe_handler.WritePacket(pipe, packet);
          if (DebugHandshake()) {
            std::cerr << "[handshake] term pipe_to_client pipe_write_ok=1\n";
          }
          did_work = true;
        } else if (packet.header() == static_cast<uint8_t>(ut::KEEP_ALIVE)) {
          connection->WritePacket(ut::Packet(static_cast<uint8_t>(ut::KEEP_ALIVE), ""));
        } else if (packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DESTINATION_REQUEST)) {
          forward_handler.HandlePacket(packet, [&](const ut::Packet& out) { connection->WritePacket(out); });
        } else if (packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DESTINATION_RESPONSE)) {
          reverse_handler.HandlePacket(packet, [&](const ut::Packet& out) { connection->WritePacket(out); });
        } else if (packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DATA)) {
          forward_handler.HandlePacket(packet, [&](const ut::Packet& out) { connection->WritePacket(out); });
          reverse_handler.HandlePacket(packet, [&](const ut::Packet& out) { connection->WritePacket(out); });
        }
        did_work = true;
      }
    }

    if (pipe_handler.HasData(pipe)) {
      ut::Packet packet;
      try {
        if (pipe_handler.ReadPacket(pipe, &packet)) {
          if (DebugHandshake()) {
            std::cerr << "[handshake] term pipe_to_client header="
                      << static_cast<int>(packet.header())
                      << " bytes=" << packet.payload().size()
                      << " jump=" << (jump_mode ? 1 : 0) << "\n";
          }
          connection->WritePacket(packet);
          if (DebugHandshake()) {
            std::cerr << "[handshake] term pipe_to_client write_ok=1\n";
          }
          did_work = true;
        } else {
          if (DebugHandshake()) {
            std::cerr << "[handshake] term pipe_to_client read_failed\n";
          }
          break;
        }
      } catch (...) {
        break;
      }
    }

    forward_handler.Update([&](const ut::Packet& out) { connection->WritePacket(out); });
    reverse_handler.Update([&](const ut::Packet& out) { connection->WritePacket(out); });
    if (!did_work) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }

  pipe_handler.Close(pipe);
  registry_->UnregisterTerminal(client_id);
  registry_->MarkActive(client_id, false);
  connection->CloseSocket();
}
