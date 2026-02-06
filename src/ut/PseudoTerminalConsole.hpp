#pragma once

class PseudoTerminalConsole {
 public:
  PseudoTerminalConsole();
  ~PseudoTerminalConsole();

  PseudoTerminalConsole(const PseudoTerminalConsole&) = delete;
  PseudoTerminalConsole& operator=(const PseudoTerminalConsole&) = delete;

  bool EnableVirtualTerminal();
  bool EnableRawInput();

 private:
  bool input_mode_valid_ = false;
  bool output_mode_valid_ = false;

#ifdef _WIN32
  void* input_handle_ = nullptr;
  void* output_handle_ = nullptr;
  unsigned long input_mode_ = 0;
  unsigned long output_mode_ = 0;
#endif
};
