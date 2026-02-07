#include "ServerClientConnection.hpp"

#include "UtConstants.hpp"

namespace ut {
ServerClientConnection::ServerClientConnection(std::shared_ptr<TcpSocketHandler> socket_handler,
                                               const std::string& client_id,
                                               const std::string& key,
                                               SocketHandle socket)
    : Connection(std::move(socket_handler), client_id, key) {
  socket_ = socket;
  reader_ = std::make_shared<BackedReader>(socket_handler_,
                                           std::make_shared<CryptoHandler>(key_, ut::kClientServerNonceMsb),
                                           socket_);
  writer_ = std::make_shared<BackedWriter>(socket_handler_,
                                           std::make_shared<CryptoHandler>(key_, ut::kServerClientNonceMsb),
                                           socket_);
}
}
