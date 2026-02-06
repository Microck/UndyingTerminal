#pragma once

#include <functional>
#include <string>

class ReconnectionManager {
 public:
  using ConnectCallback = std::function<bool(const std::string& host, int port)>;

  ReconnectionManager();

  void SetTarget(const std::string& host, int port, const std::string& client_id);
  bool AttemptReconnect(ConnectCallback connect_fn);
  void ResetBackoff();
  int GetRetryCount() const { return retry_count_; }

 private:
  std::string host_;
  int port_ = 2022;
  std::string client_id_;
  int retry_count_ = 0;
  int max_retries_ = 5;
  int base_delay_ms_ = 100;
  int max_delay_ms_ = 2000;
};
