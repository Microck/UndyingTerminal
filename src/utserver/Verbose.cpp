#include "Verbose.hpp"

namespace {
bool g_verbose = false;
}

void SetVerbose(bool verbose) {
  g_verbose = verbose;
}

bool IsVerbose() {
  return g_verbose;
}
