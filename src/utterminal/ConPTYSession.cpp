#include "ConPTYSession.hpp"

#ifdef _WIN32
#include <windows.h>

#include <atomic>
#include <thread>
#include <vector>

namespace {
struct HandlePair {
  HANDLE read = INVALID_HANDLE_VALUE;
  HANDLE write = INVALID_HANDLE_VALUE;
};

bool CreatePipePair(HandlePair* pair) {
  if (!pair) {
    return false;
  }
  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = FALSE;
  if (!CreatePipe(&pair->read, &pair->write, &sa, 0)) {
    return false;
  }
  return true;
}

COORD GetConsoleSize() {
  CONSOLE_SCREEN_BUFFER_INFO csbi{};
  if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
    return {80, 24};
  }
  const SHORT cols = static_cast<SHORT>(csbi.srWindow.Right - csbi.srWindow.Left + 1);
  const SHORT rows = static_cast<SHORT>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
  return {cols, rows};
}
}  // namespace

class ConPTYSessionState {
 public:
  HPCON conpty = nullptr;
  HandlePair conpty_input;
  HandlePair conpty_output;
  HANDLE process = INVALID_HANDLE_VALUE;
  HANDLE thread = INVALID_HANDLE_VALUE;
  std::thread output_thread;
  std::thread input_thread;
  std::thread resize_thread;
  std::atomic<bool> running{false};
  std::atomic<bool> resize_enabled{false};
};

static ConPTYSessionState g_state;

ConPTYSession::ConPTYSession() = default;

ConPTYSession::~ConPTYSession() {
  CloseHandles();
}

bool ConPTYSession::Start(const std::wstring& command_line, bool enable_resize_loop) {
  if (command_line.empty()) {
    return false;
  }

  CloseHandles();

  if (!CreatePipePair(&g_state.conpty_input) || !CreatePipePair(&g_state.conpty_output)) {
    return false;
  }

  const COORD size = GetConsoleSize();
  HRESULT hr = CreatePseudoConsole(
      size,
      g_state.conpty_input.read,
      g_state.conpty_output.write,
      0,
      &g_state.conpty);
  if (FAILED(hr)) {
    CloseHandles();
    return false;
  }

  SIZE_T attr_list_size = 0;
  InitializeProcThreadAttributeList(nullptr, 1, 0, &attr_list_size);
  std::vector<char> attr_list_buffer(attr_list_size);
  auto* attr_list = reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(attr_list_buffer.data());
  if (!InitializeProcThreadAttributeList(attr_list, 1, 0, &attr_list_size)) {
    CloseHandles();
    return false;
  }

  if (!UpdateProcThreadAttribute(
          attr_list,
          0,
          PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
          g_state.conpty,
          sizeof(g_state.conpty),
          nullptr,
          nullptr)) {
    DeleteProcThreadAttributeList(attr_list);
    CloseHandles();
    return false;
  }

  STARTUPINFOEXW startup{};
  startup.StartupInfo.cb = sizeof(startup);
  startup.lpAttributeList = attr_list;

  std::vector<wchar_t> mutable_cmd(command_line.begin(), command_line.end());
  mutable_cmd.push_back(L'\0');

  PROCESS_INFORMATION proc_info{};
  const BOOL created = CreateProcessW(
      nullptr,
      mutable_cmd.data(),
      nullptr,
      nullptr,
      FALSE,
      EXTENDED_STARTUPINFO_PRESENT,
      nullptr,
      nullptr,
      &startup.StartupInfo,
      &proc_info);

  DeleteProcThreadAttributeList(attr_list);

  if (!created) {
    CloseHandles();
    return false;
  }

  g_state.process = proc_info.hProcess;
  g_state.thread = proc_info.hThread;
  g_state.running = true;
  g_state.resize_enabled = enable_resize_loop;
  if (enable_resize_loop) {
    StartResizeLoop();
  }
  return true;
}

void ConPTYSession::Run() {
  if (!g_state.running) {
    return;
  }

  HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
  HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

  g_state.output_thread = std::thread([stdout_handle]() {
    std::vector<char> buffer(4096);
    DWORD read_bytes = 0;
    while (g_state.running && ReadFile(g_state.conpty_output.read, buffer.data(), static_cast<DWORD>(buffer.size()), &read_bytes, nullptr)) {
      if (read_bytes == 0) {
        break;
      }
      DWORD written = 0;
      WriteFile(stdout_handle, buffer.data(), read_bytes, &written, nullptr);
    }
  });

  g_state.input_thread = std::thread([stdin_handle]() {
    std::vector<char> buffer(4096);
    DWORD read_bytes = 0;
    while (g_state.running && ReadFile(stdin_handle, buffer.data(), static_cast<DWORD>(buffer.size()), &read_bytes, nullptr)) {
      if (read_bytes == 0) {
        break;
      }
      DWORD written = 0;
      WriteFile(g_state.conpty_input.write, buffer.data(), read_bytes, &written, nullptr);
    }
  });

  WaitForSingleObject(g_state.process, INFINITE);
  g_state.running = false;

  if (g_state.output_thread.joinable()) {
    g_state.output_thread.join();
  }
  if (g_state.input_thread.joinable()) {
    g_state.input_thread.join();
  }
  if (g_state.resize_thread.joinable()) {
    g_state.resize_thread.join();
  }

  CloseHandles();
}

void ConPTYSession::Wait() {
  if (!g_state.running) {
    return;
  }
  WaitForSingleObject(g_state.process, INFINITE);
  CloseHandles();
}

bool ConPTYSession::IsRunning() const {
  return g_state.running.load();
}

HANDLE ConPTYSession::InputWriteHandle() const {
  return g_state.conpty_input.write;
}

HANDLE ConPTYSession::OutputReadHandle() const {
  return g_state.conpty_output.read;
}

void ConPTYSession::Resize(short cols, short rows) {
  if (!g_state.conpty) {
    return;
  }
  COORD size{cols, rows};
  ResizePseudoConsole(g_state.conpty, size);
}

void ConPTYSession::StartResizeLoop() {
  g_state.resize_thread = std::thread([]() {
    COORD last_size = GetConsoleSize();
    while (g_state.running) {
      Sleep(200);
      COORD size = GetConsoleSize();
      if (size.X != last_size.X || size.Y != last_size.Y) {
        if (g_state.resize_enabled) {
          ResizePseudoConsole(g_state.conpty, size);
        }
        last_size = size;
      }
    }
  });
}

void ConPTYSession::CloseHandles() {
  g_state.running = false;
  if (g_state.resize_thread.joinable()) {
    g_state.resize_thread.join();
  }
  if (g_state.output_thread.joinable()) {
    g_state.output_thread.join();
  }
  if (g_state.input_thread.joinable()) {
    g_state.input_thread.join();
  }

  if (g_state.thread != INVALID_HANDLE_VALUE) {
    CloseHandle(g_state.thread);
    g_state.thread = INVALID_HANDLE_VALUE;
  }
  if (g_state.process != INVALID_HANDLE_VALUE) {
    CloseHandle(g_state.process);
    g_state.process = INVALID_HANDLE_VALUE;
  }
  if (g_state.conpty) {
    ClosePseudoConsole(g_state.conpty);
    g_state.conpty = nullptr;
  }
  if (g_state.conpty_input.read != INVALID_HANDLE_VALUE) {
    CloseHandle(g_state.conpty_input.read);
    g_state.conpty_input.read = INVALID_HANDLE_VALUE;
  }
  if (g_state.conpty_input.write != INVALID_HANDLE_VALUE) {
    CloseHandle(g_state.conpty_input.write);
    g_state.conpty_input.write = INVALID_HANDLE_VALUE;
  }
  if (g_state.conpty_output.read != INVALID_HANDLE_VALUE) {
    CloseHandle(g_state.conpty_output.read);
    g_state.conpty_output.read = INVALID_HANDLE_VALUE;
  }
  if (g_state.conpty_output.write != INVALID_HANDLE_VALUE) {
    CloseHandle(g_state.conpty_output.write);
    g_state.conpty_output.write = INVALID_HANDLE_VALUE;
  }
}
#else
ConPTYSession::ConPTYSession() = default;
ConPTYSession::~ConPTYSession() = default;

bool ConPTYSession::Start(const std::wstring& command_line, bool enable_resize_loop) {
  (void)command_line;
  (void)enable_resize_loop;
  return true;
}

void ConPTYSession::Run() {}

void ConPTYSession::Wait() {}

bool ConPTYSession::IsRunning() const { return false; }
#endif
