#include "PseudoTerminalConsole.hpp"

#ifdef _WIN32
#include <windows.h>

namespace {
bool IsValidConsoleHandle(HANDLE handle, DWORD* mode_out) {
  if (handle == INVALID_HANDLE_VALUE || handle == nullptr) {
    return false;
  }
  return GetConsoleMode(handle, mode_out) != 0;
}
}  // namespace

PseudoTerminalConsole::PseudoTerminalConsole() {
  HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
  if (IsValidConsoleHandle(input, &input_mode_)) {
    input_handle_ = input;
    input_mode_valid_ = true;
  }

  HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
  if (IsValidConsoleHandle(output, &output_mode_)) {
    output_handle_ = output;
    output_mode_valid_ = true;
  }
}

PseudoTerminalConsole::~PseudoTerminalConsole() {
  if (input_mode_valid_) {
    SetConsoleMode(static_cast<HANDLE>(input_handle_), input_mode_);
  }
  if (output_mode_valid_) {
    SetConsoleMode(static_cast<HANDLE>(output_handle_), output_mode_);
  }
}

bool PseudoTerminalConsole::EnableVirtualTerminal() {
  bool ok = true;
  if (input_mode_valid_) {
    DWORD mode = input_mode_ | ENABLE_VIRTUAL_TERMINAL_INPUT;
    ok = ok && (SetConsoleMode(static_cast<HANDLE>(input_handle_), mode) != 0);
  }
  if (output_mode_valid_) {
    DWORD mode = output_mode_ | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    ok = ok && (SetConsoleMode(static_cast<HANDLE>(output_handle_), mode) != 0);
  }
  return ok;
}

bool PseudoTerminalConsole::EnableRawInput() {
  if (!input_mode_valid_) {
    return false;
  }
  DWORD mode = input_mode_;
  mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
  mode |= ENABLE_EXTENDED_FLAGS;
  return SetConsoleMode(static_cast<HANDLE>(input_handle_), mode) != 0;
}
#else
PseudoTerminalConsole::PseudoTerminalConsole() = default;
PseudoTerminalConsole::~PseudoTerminalConsole() = default;

bool PseudoTerminalConsole::EnableVirtualTerminal() {
  return true;
}

bool PseudoTerminalConsole::EnableRawInput() {
  return true;
}
#endif
