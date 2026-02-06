#pragma once

#ifdef _WIN32
#include <winsock2.h>
#endif

class WinsockContext {
 public:
  WinsockContext();
  ~WinsockContext();

  WinsockContext(const WinsockContext&) = delete;
  WinsockContext& operator=(const WinsockContext&) = delete;

 private:
  bool initialized_ = false;
};
