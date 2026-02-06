#include "WinsockContext.hpp"

#include <iostream>

int main() {
  try {
    WinsockContext context;
    (void)context;
  } catch (const std::exception& ex) {
    std::cerr << "WinsockContext failed: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}
