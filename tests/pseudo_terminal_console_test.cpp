#include "PseudoTerminalConsole.hpp"

int main() {
  PseudoTerminalConsole console;
  console.EnableVirtualTerminal();
  console.EnableRawInput();
  return 0;
}
