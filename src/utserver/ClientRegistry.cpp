#include "ClientRegistry.hpp"

void ClientRegistry::RegisterTerminal(const std::string& client_id, const std::string& passkey, ut::SocketHandle handle) {
  ClientSession& session = sessions_[client_id];
  session.terminal_handle = handle;
  session.passkey = passkey;
  session.last_seen = std::chrono::steady_clock::now();
  session.active = true;
}

void ClientRegistry::UnregisterTerminal(const std::string& client_id) {
  sessions_.erase(client_id);
}

ut::SocketHandle ClientRegistry::LookupTerminal(const std::string& client_id) const {
  auto it = sessions_.find(client_id);
  if (it == sessions_.end()) {
    return ut::kInvalidSocket;
  }
  return it->second.terminal_handle;
}

std::string ClientRegistry::LookupPasskey(const std::string& client_id) const {
  auto it = sessions_.find(client_id);
  if (it == sessions_.end()) {
    return {};
  }
  return it->second.passkey;
}

std::shared_ptr<ut::ServerClientConnection> ClientRegistry::LookupConnection(const std::string& client_id) const {
  auto it = sessions_.find(client_id);
  if (it == sessions_.end()) {
    return nullptr;
  }
  return it->second.connection;
}

void ClientRegistry::StoreConnection(const std::string& client_id,
                                     std::shared_ptr<ut::ServerClientConnection> connection) {
  auto it = sessions_.find(client_id);
  if (it == sessions_.end()) {
    return;
  }
  it->second.connection = std::move(connection);
}

bool ClientRegistry::HasSession(const std::string& client_id) const {
  return sessions_.find(client_id) != sessions_.end();
}

void ClientRegistry::UpdateLastSeen(const std::string& client_id) {
  auto it = sessions_.find(client_id);
  if (it != sessions_.end()) {
    it->second.last_seen = std::chrono::steady_clock::now();
  }
}

void ClientRegistry::MarkActive(const std::string& client_id, bool active) {
  auto it = sessions_.find(client_id);
  if (it != sessions_.end()) {
    it->second.active = active;
  }
}

bool ClientRegistry::IsActive(const std::string& client_id) const {
  auto it = sessions_.find(client_id);
  if (it == sessions_.end()) {
    return false;
  }
  return it->second.active;
}

void ClientRegistry::CleanupStale(int timeout_seconds) {
  auto now = std::chrono::steady_clock::now();
  for (auto it = sessions_.begin(); it != sessions_.end();) {
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_seen).count();
    if (!it->second.active && elapsed > timeout_seconds) {
      it = sessions_.erase(it);
    } else {
      ++it;
    }
  }
}
