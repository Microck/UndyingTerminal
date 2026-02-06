#pragma once

#include <memory>

#include "Connection.hpp"
#include "TcpSocketHandler.hpp"

namespace ut {
class ServerClientConnection : public Connection {
 public:
  ServerClientConnection(std::shared_ptr<TcpSocketHandler> socket_handler,
                         const std::string& client_id,
                         const std::string& key,
                         SocketHandle socket);
};
}
