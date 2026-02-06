#pragma once

#include <atomic>
#include <functional>
#include <thread>

class Keepalive {
 public:
  using SendCallback = std::function<bool()>;

  Keepalive();
  ~Keepalive();

  void Start(int interval_seconds, SendCallback send_fn);
  void Stop();
  void Reset();
  bool IsConnectionDead() const;

 private:
  std::atomic<bool> running_{false};
  std::atomic<bool> connection_dead_{false};
  std::atomic<int> missed_count_{0};
  std::thread thread_;
  int interval_seconds_ = 5;
  int max_missed_ = 3;
};
