#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

#include "et/SocketTypes.hpp"

namespace ut {
class ServerClientConnection;
}

struct ClientSession {
  ut::SocketHandle terminal_handle = ut::kInvalidSocket;
  std::string passkey;
  std::shared_ptr<ut::ServerClientConnection> connection;
  std::chrono::steady_clock::time_point last_seen;
  bool active = false;
};

class ClientRegistry {
 public:
  void RegisterTerminal(const std::string& client_id, const std::string& passkey, ut::SocketHandle handle);
  void UnregisterTerminal(const std::string& client_id);
  ut::SocketHandle LookupTerminal(const std::string& client_id) const;
  std::string LookupPasskey(const std::string& client_id) const;
  std::shared_ptr<ut::ServerClientConnection> LookupConnection(const std::string& client_id) const;
  void StoreConnection(const std::string& client_id, std::shared_ptr<ut::ServerClientConnection> connection);

  bool HasSession(const std::string& client_id) const;
  void UpdateLastSeen(const std::string& client_id);
  void MarkActive(const std::string& client_id, bool active);
  bool IsActive(const std::string& client_id) const;
  void CleanupStale(int timeout_seconds);

 private:
  std::unordered_map<std::string, ClientSession> sessions_;
};
