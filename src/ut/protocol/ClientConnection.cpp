#include "ClientConnection.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>

#include "UtConstants.hpp"
#include "UT.pb.h"

namespace ut {
namespace {
bool DebugHandshake() {
  return std::getenv("UT_DEBUG_HANDSHAKE") != nullptr;
}
}
ClientConnection::ClientConnection(std::shared_ptr<TcpSocketHandler> socket_handler,
                                    const ut::SocketEndpoint& remote,
                                    const std::string& id,
                                    const std::string& key)
    : Connection(socket_handler, id, key), tcp_handler_(std::move(socket_handler)), remote_(remote) {}

ClientConnection::~ClientConnection() {
  WaitReconnect();
  CloseSocket();
}

bool ClientConnection::Connect() {
  try {
    socket_ = tcp_handler_->Connect(remote_.name(), remote_.port());
    if (socket_ == kInvalidSocket) {
      return false;
    }
    ut::ConnectRequest request;
    request.set_clientid(id_);
    request.set_version(ut::kProtocolVersion);
    socket_handler_->WriteProto(socket_, request, true);
    ut::ConnectResponse response = socket_handler_->ReadProto<ut::ConnectResponse>(socket_, true);
    returning_client_ = response.status() == ut::RETURNING_CLIENT;
    if (DebugHandshake()) {
      std::cerr << "[handshake] connect_response status=" << response.status()
                << " client_id_len=" << id_.size()
                << " key_len=" << key_.size() << "\n";
    }
    if (response.status() != ut::NEW_CLIENT && response.status() != ut::RETURNING_CLIENT) {
      socket_handler_->Close(socket_);
      socket_ = kInvalidSocket;
      return false;
    }
    reader_ = std::make_shared<BackedReader>(socket_handler_,
                                             std::make_shared<CryptoHandler>(key_, ut::kServerClientNonceMsb),
                                             socket_);
    writer_ = std::make_shared<BackedWriter>(socket_handler_,
                                             std::make_shared<CryptoHandler>(key_, ut::kClientServerNonceMsb),
                                             socket_);
    if (returning_client_) {
      if (!Recover(socket_)) {
        socket_handler_->Close(socket_);
        socket_ = kInvalidSocket;
        return false;
      }
    }
    return true;
  } catch (...) {
    if (socket_ != kInvalidSocket) {
      socket_handler_->Close(socket_);
    }
    socket_ = kInvalidSocket;
  }
  return false;
}

 void ClientConnection::CloseSocketAndMaybeReconnect() {
   WaitReconnect();
   CloseSocket();
   if (!shutting_down_ && reconnect_enabled_) {
     reconnect_thread_ = std::make_shared<std::thread>(&ClientConnection::PollReconnect, this);
   }
 }

void ClientConnection::WaitReconnect() {
  if (reconnect_thread_) {
    reconnect_thread_->join();
    reconnect_thread_.reset();
  }
}

 void ClientConnection::PollReconnect() {
   while (socket_ == kInvalidSocket) {
     {
       std::lock_guard<std::recursive_mutex> guard(mutex_);
       if (shutting_down_ || !reconnect_enabled_) {
         return;
       }
      SocketHandle new_socket = tcp_handler_->Connect(remote_.name(), remote_.port());
      if (new_socket != kInvalidSocket) {
        try {
          ut::ConnectRequest request;
          request.set_clientid(id_);
          request.set_version(ut::kProtocolVersion);
          socket_handler_->WriteProto(new_socket, request, true);
          ut::ConnectResponse response = socket_handler_->ReadProto<ut::ConnectResponse>(new_socket, true);
          if (response.status() == ut::INVALID_KEY) {
            socket_handler_->Close(new_socket);
            shutting_down_ = true;
            return;
          }
          if (response.status() != ut::RETURNING_CLIENT) {
            socket_handler_->Close(new_socket);
          } else {
            Recover(new_socket);
          }
        } catch (...) {
          socket_handler_->Close(new_socket);
        }
      }
    }
    if (socket_ == kInvalidSocket) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}
}
