#include "Keepalive.hpp"

#include <chrono>

#ifdef _WIN32
#include <windows.h>
#define SLEEP_SECONDS(s) Sleep((s) * 1000)
#else
#include <unistd.h>
#define SLEEP_SECONDS(s) sleep(s)
#endif

Keepalive::Keepalive() = default;

Keepalive::~Keepalive() {
  Stop();
}

void Keepalive::Start(int interval_seconds, SendCallback send_fn) {
  if (running_) {
    return;
  }

  interval_seconds_ = interval_seconds;
  running_ = true;
  connection_dead_ = false;
  missed_count_ = 0;

  thread_ = std::thread([this, send_fn]() {
    while (running_) {
      SLEEP_SECONDS(interval_seconds_);
      if (!running_) {
        break;
      }

      if (send_fn && send_fn()) {
        missed_count_ = 0;
      } else {
        ++missed_count_;
        if (missed_count_ >= max_missed_) {
          connection_dead_ = true;
        }
      }
    }
  });
}

void Keepalive::Stop() {
  running_ = false;
  if (thread_.joinable()) {
    thread_.join();
  }
}

void Keepalive::Reset() {
  missed_count_ = 0;
  connection_dead_ = false;
}

bool Keepalive::IsConnectionDead() const {
  return connection_dead_;
}
