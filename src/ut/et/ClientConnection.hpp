#pragma once

#include <memory>
#include <thread>

#include "Connection.hpp"
#include "TcpSocketHandler.hpp"

#include "ET.pb.h"

namespace ut {
class ClientConnection : public Connection {
 public:
  ClientConnection(std::shared_ptr<TcpSocketHandler> socket_handler,
                   const et::SocketEndpoint& remote,
                   const std::string& id,
                   const std::string& key);
  ~ClientConnection() override;

   bool Connect();
   void CloseSocketAndMaybeReconnect() override;

   void SetReconnectEnabled(bool enabled) { reconnect_enabled_ = enabled; }
   bool IsReconnectEnabled() const { return reconnect_enabled_; }
   bool IsReturningClient() const { return returning_client_; }

 private:
  void PollReconnect();
  void WaitReconnect();

   std::shared_ptr<TcpSocketHandler> tcp_handler_;
   et::SocketEndpoint remote_;
   std::shared_ptr<std::thread> reconnect_thread_;
   bool reconnect_enabled_ = true;
   bool returning_client_ = false;
};
}
