#include "ReconnectionManager.hpp"

#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <unistd.h>
#define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

ReconnectionManager::ReconnectionManager() = default;

void ReconnectionManager::SetTarget(const std::string& host, int port, const std::string& client_id) {
  host_ = host;
  port_ = port;
  client_id_ = client_id;
}

bool ReconnectionManager::AttemptReconnect(ConnectCallback connect_fn) {
  if (!connect_fn) {
    return false;
  }

  while (retry_count_ < max_retries_) {
    int delay = base_delay_ms_ * (1 << retry_count_);
    if (delay > max_delay_ms_) {
      delay = max_delay_ms_;
    }

    if (retry_count_ > 0) {
      SLEEP_MS(delay);
    }

    ++retry_count_;

    if (connect_fn(host_, port_)) {
      return true;
    }
  }

  return false;
}

void ReconnectionManager::ResetBackoff() {
  retry_count_ = 0;
}
