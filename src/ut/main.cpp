#include <atomic>
#include <chrono>
#include <exception>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "ClientId.hpp"
#include "PseudoTerminalConsole.hpp"
#include "SshConfig.hpp"
#include "SshCommandBuilder.hpp"
#include "SshSubprocess.hpp"
#include "WinsockContext.hpp"

#include "protocol/ClientConnection.hpp"
#include "protocol/Packet.hpp"
#include "protocol/PortForwardHandler.hpp"
#include "protocol/TcpSocketHandler.hpp"
#include "protocol/TunnelUtils.hpp"
#include "UtConstants.hpp"
#include "UT.pb.h"
#include "UTerminal.pb.h"

namespace {
bool DebugHandshake() {
  return std::getenv("UT_DEBUG_HANDSHAKE") != nullptr;
}
std::string GenerateRandom(size_t len) {
  static const char kChars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<size_t> dist(0, sizeof(kChars) - 2);
  std::string out;
  out.reserve(len);
  for (size_t i = 0; i < len; ++i) {
    out.push_back(kChars[dist(gen)]);
  }
  return out;
}

std::string NormalizeCommand(const std::string& command) {
  if (command.empty()) {
    return command;
  }
  const char last = command.back();
  if (last == '\n' || last == '\r') {
    return command;
  }
  return command + "\r\n";
}

bool ReadLineFromSsh(SshSubprocess& ssh, std::string* out) {
  if (!out) {
    return false;
  }
  std::string buffer;
  std::string chunk;
  while (ssh.Read(&chunk)) {
    buffer.append(chunk);
    size_t newline = buffer.find('\n');
    if (newline != std::string::npos) {
      *out = buffer.substr(0, newline);
      if (!out->empty() && out->back() == '\r') {
        out->pop_back();
      }
      return true;
    }
  }
  return false;
}

int64_t NowMs() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

COORD GetConsoleSize() {
  CONSOLE_SCREEN_BUFFER_INFO csbi{};
  if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
    return {80, 24};
  }
  SHORT cols = static_cast<SHORT>(csbi.srWindow.Right - csbi.srWindow.Left + 1);
  SHORT rows = static_cast<SHORT>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
  return {cols, rows};
}

bool SendTerminalInfo(ut::ClientConnection& connection, const std::string& client_id, short cols, short rows) {
  ut::TerminalInfo info;
  info.set_id(client_id);
  info.set_width(cols);
  info.set_height(rows);
  std::string payload;
  if (!info.SerializeToString(&payload)) {
    return false;
  }
  ut::Packet packet(static_cast<uint8_t>(ut::TERMINAL_INFO), payload);
  connection.WritePacket(packet);
  return true;
}

std::string EscapeForDoubleQuotes(const std::string& input) {
  std::string out;
  out.reserve(input.size());
  for (char c : input) {
    if (c == '\\' || c == '"') {
      out.push_back('\\');
    }
    out.push_back(c);
  }
  return out;
}

std::string WrapWithTmux(const std::string& command, const std::string& session_name) {
  const std::string escaped = EscapeForDoubleQuotes(command);
  return "tmux new-session -A -s '" + session_name + "' \"" + escaped + "\"";
}

class PredictiveEcho {
 public:
  PredictiveEcho(bool enabled, HANDLE stdout_handle)
      : enabled_(enabled), stdout_handle_(stdout_handle) {}

  void OnLocalInput(const char* data, size_t len) {
    if (!enabled_ || !data || len == 0) {
      return;
    }
    std::string display;
    display.reserve(len * 3);
    {
      std::lock_guard<std::mutex> lock(mu_);
      for (size_t i = 0; i < len; ++i) {
        const unsigned char c = static_cast<unsigned char>(data[i]);
        if (c == 8 || c == 127) {
          if (!pending_.empty()) {
            pending_.pop_back();
            display += "\b \b";
          }
          continue;
        }
        if (c >= 32 && c <= 126) {
          pending_.push_back(static_cast<char>(c));
          display.push_back(static_cast<char>(c));
        }
      }
      if (pending_.size() > 4096) {
        pending_.erase(0, pending_.size() - 4096);
      }
    }
    if (!display.empty()) {
      DWORD written = 0;
      WriteFile(stdout_handle_, display.data(), static_cast<DWORD>(display.size()), &written, nullptr);
    }
  }

  void OnRemoteOutput(std::string* output) {
    if (!enabled_ || !output || output->empty()) {
      return;
    }
    std::lock_guard<std::mutex> lock(mu_);
    size_t match = 0;
    while (match < output->size() && match < pending_.size() && (*output)[match] == pending_[match]) {
      ++match;
    }
    if (match > 0) {
      pending_.erase(0, match);
      output->erase(0, match);
      return;
    }
    if (!pending_.empty()) {
      pending_.clear();
    }
  }

 private:
  bool enabled_ = false;
  HANDLE stdout_handle_ = INVALID_HANDLE_VALUE;
  std::mutex mu_;
  std::string pending_;
};

struct UiSessionProfile {
  std::string name;
  std::string host;
  int port = 2022;
  std::string client_id;
  std::string passkey;
  std::string tunnel;
  bool tunnel_only = false;
  bool noexit = false;
  bool predictive_echo = false;
};

std::string QuoteWindowsArg(const std::string& value) {
  if (value.find_first_of(" \t\"") == std::string::npos) {
    return value;
  }
  std::string out = "\"";
  for (char c : value) {
    if (c == '"') {
      out += "\\\"";
    } else {
      out.push_back(c);
    }
  }
  out += "\"";
  return out;
}

bool StartUiSessionProcess(const std::string& exe_path,
                           const UiSessionProfile& profile,
                           PROCESS_INFORMATION* out_process,
                           std::string* out_error) {
  if (!out_process) {
    return false;
  }

  std::ostringstream command;
  command << QuoteWindowsArg(exe_path) << " --connect "
          << QuoteWindowsArg(profile.host) << " "
          << profile.port << " "
          << QuoteWindowsArg(profile.client_id) << " --key "
          << QuoteWindowsArg(profile.passkey);
  if (!profile.tunnel.empty()) {
    command << " --tunnel " << QuoteWindowsArg(profile.tunnel);
  }
  if (profile.tunnel_only) {
    command << " --tunnel-only";
  }
  if (profile.noexit) {
    command << " --noexit";
  }
  if (profile.predictive_echo) {
    command << " --predictive-echo";
  }

  std::string command_line = command.str();
  std::vector<char> command_buf(command_line.begin(), command_line.end());
  command_buf.push_back('\0');

  STARTUPINFOA startup{};
  startup.cb = sizeof(startup);
  PROCESS_INFORMATION process{};
  if (!CreateProcessA(nullptr,
                      command_buf.data(),
                      nullptr,
                      nullptr,
                      FALSE,
                      CREATE_NEW_CONSOLE,
                      nullptr,
                      nullptr,
                      &startup,
                      &process)) {
    if (out_error) {
      *out_error = "CreateProcess failed with error " + std::to_string(GetLastError());
    }
    return false;
  }

  *out_process = process;
  return true;
}

void PrintUiHelp() {
  std::cout << "Built-in UI commands:\n"
            << "  help\n"
            << "  add <name> <host> <port> <client_id> <passkey>\n"
            << "  set-tunnel <name> <spec>\n"
            << "  set-flag <name> <noexit|tunnel-only|predictive-echo> <on|off>\n"
            << "  start <name>\n"
            << "  stop <name>\n"
            << "  remove <name>\n"
            << "  list\n"
            << "  quit\n";
}

int RunBuiltInUi(const std::string& exe_path) {
  std::map<std::string, UiSessionProfile> profiles;
  std::unordered_map<std::string, PROCESS_INFORMATION> running;

  std::cout << "Undying Terminal built-in UI\n";
  PrintUiHelp();

  std::string line;
  while (true) {
    std::cout << "ut-ui> " << std::flush;
    if (!std::getline(std::cin, line)) {
      break;
    }
    if (line.empty()) {
      continue;
    }

    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd == "help") {
      PrintUiHelp();
      continue;
    }

    if (cmd == "add") {
      UiSessionProfile profile;
      iss >> profile.name >> profile.host >> profile.port >> profile.client_id >> profile.passkey;
      if (profile.name.empty() || profile.host.empty() || profile.client_id.empty() || profile.passkey.empty()) {
        std::cout << "usage: add <name> <host> <port> <client_id> <passkey>\n";
        continue;
      }
      profiles[profile.name] = profile;
      std::cout << "saved profile '" << profile.name << "'\n";
      continue;
    }

    if (cmd == "set-tunnel") {
      std::string name;
      iss >> name;
      std::string tunnel;
      std::getline(iss, tunnel);
      if (!tunnel.empty() && tunnel.front() == ' ') {
        tunnel.erase(0, 1);
      }
      auto it = profiles.find(name);
      if (it == profiles.end()) {
        std::cout << "unknown profile\n";
        continue;
      }
      it->second.tunnel = tunnel;
      std::cout << "updated tunnel for '" << name << "'\n";
      continue;
    }

    if (cmd == "set-flag") {
      std::string name;
      std::string flag;
      std::string value;
      iss >> name >> flag >> value;
      auto it = profiles.find(name);
      if (it == profiles.end()) {
        std::cout << "unknown profile\n";
        continue;
      }
      const bool enabled = (value == "on" || value == "true" || value == "1");
      if (flag == "noexit") {
        it->second.noexit = enabled;
      } else if (flag == "tunnel-only") {
        it->second.tunnel_only = enabled;
      } else if (flag == "predictive-echo") {
        it->second.predictive_echo = enabled;
      } else {
        std::cout << "unknown flag\n";
        continue;
      }
      std::cout << "updated flag for '" << name << "'\n";
      continue;
    }

    if (cmd == "start") {
      std::string name;
      iss >> name;
      auto it = profiles.find(name);
      if (it == profiles.end()) {
        std::cout << "unknown profile\n";
        continue;
      }
      if (running.find(name) != running.end()) {
        std::cout << "profile already running\n";
        continue;
      }
      PROCESS_INFORMATION process{};
      std::string error;
      if (!StartUiSessionProcess(exe_path, it->second, &process, &error)) {
        std::cout << "start failed: " << error << "\n";
        continue;
      }
      CloseHandle(process.hThread);
      running[name] = process;
      std::cout << "started '" << name << "' (pid=" << process.dwProcessId << ")\n";
      continue;
    }

    if (cmd == "stop") {
      std::string name;
      iss >> name;
      auto it = running.find(name);
      if (it == running.end()) {
        std::cout << "profile is not running\n";
        continue;
      }
      TerminateProcess(it->second.hProcess, 0);
      CloseHandle(it->second.hProcess);
      running.erase(it);
      std::cout << "stopped '" << name << "'\n";
      continue;
    }

    if (cmd == "remove") {
      std::string name;
      iss >> name;
      if (running.find(name) != running.end()) {
        std::cout << "stop session before removing\n";
        continue;
      }
      if (profiles.erase(name) == 0) {
        std::cout << "unknown profile\n";
      } else {
        std::cout << "removed profile '" << name << "'\n";
      }
      continue;
    }

    if (cmd == "list") {
      if (profiles.empty()) {
        std::cout << "no profiles\n";
        continue;
      }
      for (const auto& entry : profiles) {
        const bool is_running = running.find(entry.first) != running.end();
        std::cout << "- " << entry.first
                  << " host=" << entry.second.host
                  << " port=" << entry.second.port
                  << " running=" << (is_running ? "yes" : "no")
                  << " tunnel-only=" << (entry.second.tunnel_only ? "on" : "off")
                  << " predictive-echo=" << (entry.second.predictive_echo ? "on" : "off")
                  << "\n";
      }
      continue;
    }

    if (cmd == "quit" || cmd == "exit") {
      break;
    }

    std::cout << "unknown command\n";
  }

  for (auto& entry : running) {
    TerminateProcess(entry.second.hProcess, 0);
    CloseHandle(entry.second.hProcess);
  }
  return 0;
}
}

int main(int argc, char** argv) {
  if (argc > 1 && std::string(argv[1]) == "--ui") {
    return RunBuiltInUi(argv[0]);
  }

  if (argc > 1 && std::string(argv[1]) == "--version") {
    std::cout << "Undying Terminal undying-terminal.exe " << UNDYING_TERMINAL_VERSION << "\n";
    return 0;
  }

  if (argc > 1 && std::string(argv[1]) == "--self-test") {
    try {
      WinsockContext winsock;
      (void)winsock;
      std::cout << "Self-test passed\n";
      return 0;
    } catch (const std::exception& ex) {
      std::cerr << "Winsock init failed: " << ex.what() << "\n";
      return 1;
    }
  }

  if (argc > 2 && std::string(argv[1]) == "--ssh") {
    std::string host = argv[2];
    std::string user;
    int ssh_port = 22;
    bool ssh_port_set = false;
    bool user_set = false;
    int server_port = 2022;
    std::string identity;
    bool identity_set = false;
    std::string remote_terminal = "undying-terminal-terminal.exe";
    std::string tunnel_arg;
    std::string reverse_tunnel_arg;
    std::string jumphost_arg;
    int jport_arg = 2022;
    std::string command_arg;
    bool noexit = false;
    bool tunnel_only = false;
    bool predictive_echo = false;
    bool tmux_enabled = false;
    std::string tmux_session = "undying-terminal";
    bool ssh_config_enabled = true;
    std::string ssh_config_path;
    bool ssh_agent_enabled = false;
    bool ssh_agent_set = false;
    std::string ssh_proxy_jump;
    std::vector<std::string> config_local_forwards;
 
     for (int i = 3; i < argc; ++i) {
       std::string arg = argv[i];
        if (arg == "-l" && i + 1 < argc) {
          user = argv[++i];
          user_set = true;
          continue;
        }
        if (arg == "-p" && i + 1 < argc) {
          ssh_port = std::stoi(argv[++i]);
          ssh_port_set = true;
          continue;
        }
       if (arg == "--server-port" && i + 1 < argc) {
         server_port = std::stoi(argv[++i]);
         continue;
       }
        if (arg == "-i" && i + 1 < argc) {
          identity = argv[++i];
          identity_set = true;
          continue;
        }
        if (arg == "--remote-terminal" && i + 1 < argc) {
          remote_terminal = argv[++i];
          continue;
        }
        if (arg == "--ssh-config" && i + 1 < argc) {
          ssh_config_path = argv[++i];
          continue;
        }
        if (arg == "--no-ssh-config") {
          ssh_config_enabled = false;
          continue;
        }
        if (arg == "--ssh-agent" || arg == "-A") {
          ssh_agent_enabled = true;
          ssh_agent_set = true;
          continue;
        }
        if (arg == "--no-ssh-agent") {
          ssh_agent_enabled = false;
          ssh_agent_set = true;
          continue;
        }
      if ((arg == "--jumphost" || arg == "-jumphost") && i + 1 < argc) {
        jumphost_arg = argv[++i];
        continue;
      }
      if ((arg == "--jport" || arg == "-jport") && i + 1 < argc) {
        jport_arg = std::stoi(argv[++i]);
        continue;
      }
      if ((arg == "-t" || arg == "--tunnel") && i + 1 < argc) {
        tunnel_arg = argv[++i];
        continue;
      }
      if ((arg == "-r" || arg == "--reversetunnel") && i + 1 < argc) {
        reverse_tunnel_arg = argv[++i];
        continue;
      }
      if ((arg == "-c" || arg == "--command") && i + 1 < argc) {
        command_arg = argv[++i];
        continue;
      }
      if (arg == "--predictive-echo") {
        predictive_echo = true;
        continue;
      }
      if (arg == "--noexit") {
        noexit = true;
        continue;
      }
      if (arg == "--tunnel-only") {
        tunnel_only = true;
        continue;
      }
      if (arg == "--tmux") {
        tmux_enabled = true;
        continue;
      }
      if (arg == "--tmux-session" && i + 1 < argc) {
        tmux_session = argv[++i];
        continue;
      }
     }

    if (ssh_config_enabled) {
      const std::string config_path = ssh_config_path.empty()
                                          ? SshConfig::DefaultConfigPath()
                                          : ssh_config_path;
      const bool optional = ssh_config_path.empty();
      SshConfigOptions config;
      std::string config_error;
      if (!SshConfig::LoadForHost(host, config_path, optional, &config, &config_error)) {
        std::cerr << "SSH config error: " << config_error << "\n";
        return 1;
      }
      if (!config.host_name.empty()) {
        host = config.host_name;
      }
      if (!user_set && !config.user.empty()) {
        user = config.user;
      }
      if (!ssh_port_set && config.port > 0) {
        ssh_port = config.port;
      }
      if (!identity_set && !config.identity_file.empty()) {
        identity = config.identity_file;
      }
      if (!config.proxy_jump.empty()) {
        ssh_proxy_jump = config.proxy_jump;
      }
      if (config.forward_agent_set && !ssh_agent_set) {
        ssh_agent_enabled = config.forward_agent;
        ssh_agent_set = true;
      }
      config_local_forwards = config.local_forwards;
    }

    if (tunnel_only && !command_arg.empty()) {
      std::cerr << "--tunnel-only cannot be combined with --command\n";
      return 1;
    }

    const std::string seed_id = "XXX" + GenerateRandom(13);
    const std::string seed_key = GenerateRandom(32);
    const char* term_env = std::getenv("TERM");
    const std::string term = term_env ? term_env : "xterm-256color";
    const std::string idpasskey = seed_id + "/" + seed_key + "_" + term;
    std::string remote_cmd = "echo '" + idpasskey + "' | " + remote_terminal;
    if (tunnel_only) {
      remote_cmd += " --tunnel-only";
    }
    if (tmux_enabled) {
      remote_cmd = WrapWithTmux(remote_cmd, tmux_session);
    }

    SshCommandBuilder builder;
    builder.SetHost(host).SetUser(user).SetPort(ssh_port).SetRemoteCommand(remote_cmd);
    if (!identity.empty()) {
      builder.SetIdentityFile(identity);
    }
    if (!ssh_proxy_jump.empty()) {
      builder.AddOption("-J " + ssh_proxy_jump);
    }
    if (ssh_agent_enabled) {
      builder.AddOption("-A");
    }
    if (!jumphost_arg.empty()) {
      builder.AddOption("-J " + jumphost_arg);
    }

    SshSubprocess ssh;
    if (!ssh.Start(builder.Build())) {
      std::cerr << "Failed to start SSH\n";
      return 1;
    }

    std::string idpasskey_reply;
    if (!ReadLineFromSsh(ssh, &idpasskey_reply)) {
      idpasskey_reply = seed_id + "/" + seed_key;
    }

    ssh.Terminate();

    auto slash_pos = idpasskey_reply.find('/');
    if (slash_pos == std::string::npos) {
      std::cerr << "Invalid id/passkey response\n";
      return 1;
    }
    std::string client_id = idpasskey_reply.substr(0, slash_pos);
    std::string passkey = idpasskey_reply.substr(slash_pos + 1);

    SshSubprocess jump_ssh;
    bool jump_active = false;
    if (!jumphost_arg.empty()) {
      std::string jump_cmd = "echo '" + client_id + "/" + passkey + "' | " + remote_terminal + " --jump";
      if (tunnel_only) {
        jump_cmd += " --tunnel-only";
      }
      if (tmux_enabled) {
        jump_cmd = WrapWithTmux(jump_cmd, tmux_session + "-jump");
      }
      SshCommandBuilder jump_builder;
      jump_builder.SetHost(jumphost_arg).SetUser(user).SetPort(ssh_port).SetRemoteCommand(jump_cmd);
      if (!identity.empty()) {
        jump_builder.SetIdentityFile(identity);
      }
      if (ssh_agent_enabled) {
        jump_builder.AddOption("-A");
      }
      if (!jump_ssh.Start(jump_builder.Build())) {
        std::cerr << "Failed to start jumphost terminal\n";
        return 1;
      }
      jump_active = true;
    }

    ut::SocketEndpoint endpoint;
    if (!jumphost_arg.empty()) {
      endpoint.set_name(jumphost_arg);
      endpoint.set_port(jport_arg);
    } else {
      endpoint.set_name(host);
      endpoint.set_port(server_port);
    }

    const bool interactive = !tunnel_only && (command_arg.empty() || noexit);
    const bool enable_keepalive = interactive || tunnel_only;
    const bool enable_terminal_output = !tunnel_only;

    std::vector<ut::PortForwardSourceRequest> forward_requests;
    auto append_forward_requests = [&](const std::string& arg) {
      for (const auto& req : ut::ParseRangesToRequests(arg)) {
        forward_requests.push_back(req);
      }
    };
    try {
      for (const auto& forward_arg : config_local_forwards) {
        append_forward_requests(forward_arg);
      }
      if (!tunnel_arg.empty()) {
        append_forward_requests(tunnel_arg);
      }
    } catch (const std::exception& ex) {
      std::cerr << "Tunnel parse failed: " << ex.what() << "\n";
      return 1;
    }
 
    auto socket_handler = std::make_shared<ut::TcpSocketHandler>();
    ut::ClientConnection connection(socket_handler, endpoint, client_id, passkey);
    connection.SetReconnectEnabled(interactive);
    if (!connection.Connect()) {
       std::cerr << "Failed to connect to server\n";
       return 1;
     }

    const bool returning_client = connection.IsReturningClient();
    if (!returning_client) {
      ut::InitialPayload payload;
      if (!jumphost_arg.empty()) {
        payload.set_jumphost(true);
        (*payload.mutable_environmentvariables())["dsthost"] = host;
        (*payload.mutable_environmentvariables())["dstport"] = std::to_string(server_port);
      } else {
        payload.set_jumphost(false);
      }
      if (!reverse_tunnel_arg.empty()) {
        try {
          for (const auto& req : ut::ParseRangesToRequests(reverse_tunnel_arg)) {
            *payload.add_reversetunnels() = req;
          }
        } catch (const std::exception& ex) {
          std::cerr << "Reverse tunnel parse failed: " << ex.what() << "\n";
          return 1;
        }
      }
      std::string payload_bytes;
      payload.SerializeToString(&payload_bytes);
      connection.WritePacket(ut::Packet(static_cast<uint8_t>(ut::INITIAL_PAYLOAD), payload_bytes));

      ut::Packet response_packet;
      if (!connection.ReadPacket(&response_packet) || response_packet.header() != static_cast<uint8_t>(ut::INITIAL_RESPONSE)) {
        std::cerr << "Missing initial response\n";
        return 1;
      }
      ut::InitialResponse response;
      if (!response.ParseFromString(response_packet.payload()) || !response.error().empty()) {
        std::cerr << "Initial response error: " << response.error() << "\n";
        return 1;
      }

      if (!command_arg.empty()) {
        ut::TerminalBuffer tb;
        tb.set_buffer(NormalizeCommand(command_arg));
        std::string tb_bytes;
        if (tb.SerializeToString(&tb_bytes)) {
          connection.WritePacket(ut::Packet(static_cast<uint8_t>(ut::TERMINAL_BUFFER), tb_bytes));
        }
      }
      if (!noexit && !command_arg.empty()) {
        ut::TerminalBuffer tb;
        tb.set_buffer("exit\r\n");
        std::string tb_bytes;
        if (tb.SerializeToString(&tb_bytes)) {
          connection.WritePacket(ut::Packet(static_cast<uint8_t>(ut::TERMINAL_BUFFER), tb_bytes));
        }
      }
    }
 
     PseudoTerminalConsole console;
     if (interactive) {
       console.EnableVirtualTerminal();
       console.EnableRawInput();
     }

    HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    PredictiveEcho predictor(predictive_echo && interactive, stdout_handle);
    std::atomic<bool> running{true};
    std::atomic<int64_t> last_rx_ms{NowMs()};

    constexpr int64_t kKeepaliveIntervalMs = 5000;
    constexpr int64_t kKeepaliveDeadMs = 15000;
    std::thread keepalive_thread;
    if (enable_keepalive) {
      keepalive_thread = std::thread([&]() {
        while (running) {
          Sleep(static_cast<DWORD>(kKeepaliveIntervalMs));
          if (!running) {
            break;
          }
          connection.Write(ut::Packet(static_cast<uint8_t>(ut::KEEP_ALIVE), ""));
          const int64_t age = NowMs() - last_rx_ms.load();
          if (age > kKeepaliveDeadMs) {
            connection.CloseSocketAndMaybeReconnect();
            last_rx_ms.store(NowMs());
          }
        }
      });
    }

    std::shared_ptr<ut::PortForwardHandler> forward_handler;
    if (!forward_requests.empty()) {
      forward_handler = std::make_shared<ut::PortForwardHandler>(socket_handler, false);
      try {
        for (const auto& req : forward_requests) {
          forward_handler->AddForwardRequest(req);
        }
      } catch (const std::exception& ex) {
        std::cerr << "Tunnel parse failed: " << ex.what() << "\n";
        return 1;
      }
    }
    std::shared_ptr<ut::PortForwardHandler> reverse_handler;
    if (!reverse_tunnel_arg.empty()) {
      reverse_handler = std::make_shared<ut::PortForwardHandler>(socket_handler, true);
    }

    auto send_packet = [&](const ut::Packet& packet) { connection.WritePacket(packet); };

    std::thread input_thread;
    if (interactive) {
      input_thread = std::thread([&]() {
        std::vector<char> buffer(4096);
        DWORD read_bytes = 0;
        while (running && ReadFile(stdin_handle, buffer.data(), static_cast<DWORD>(buffer.size()), &read_bytes, nullptr)) {
          if (read_bytes == 0) {
            break;
          }
          predictor.OnLocalInput(buffer.data(), static_cast<size_t>(read_bytes));
          ut::TerminalBuffer tb;
          tb.set_buffer(std::string(buffer.data(), buffer.data() + read_bytes));
          std::string tb_bytes;
          if (!tb.SerializeToString(&tb_bytes)) {
            continue;
          }
          connection.WritePacket(ut::Packet(static_cast<uint8_t>(ut::TERMINAL_BUFFER), tb_bytes));
        }
        running = false;
      });
    }

    std::thread output_thread([&]() {
      ut::Packet packet;
      if (!interactive && !tunnel_only) {
        const int64_t start_ms = NowMs();
        int64_t last_output_ms = start_ms;
        bool saw_output = false;
        constexpr int64_t kFirstOutputTimeoutMs = 5000;
        constexpr int64_t kIdleExitMs = 500;

        while (running) {
          auto reader = connection.reader();
          if (!reader || !reader->HasData()) {
            const int64_t now = NowMs();
            if (!saw_output && now - start_ms > kFirstOutputTimeoutMs) {
              break;
            }
            if (saw_output && now - last_output_ms > kIdleExitMs) {
              break;
            }
            Sleep(5);
            continue;
          }

          if (!connection.ReadPacket(&packet)) {
            Sleep(5);
            continue;
          }
          if (packet.header() == static_cast<uint8_t>(ut::KEEP_ALIVE)) {
            continue;
          }
          if (packet.header() == static_cast<uint8_t>(ut::TERMINAL_BUFFER)) {
            if (!enable_terminal_output) {
              continue;
            }
            ut::TerminalBuffer tb;
            if (!tb.ParseFromString(packet.payload())) {
              continue;
            }
            std::string output = tb.buffer();
            predictor.OnRemoteOutput(&output);
            if (output.empty()) {
              continue;
            }
            DWORD written = 0;
            WriteFile(stdout_handle,
                      output.data(),
                      static_cast<DWORD>(output.size()),
                      &written,
                      nullptr);
            saw_output = true;
            last_output_ms = NowMs();
          } else if (packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DESTINATION_REQUEST)) {
            if (reverse_handler) {
              reverse_handler->HandlePacket(packet, send_packet);
            }
          } else if (packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DESTINATION_RESPONSE) ||
                     packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DATA)) {
            if (forward_handler) {
              forward_handler->HandlePacket(packet, send_packet);
            }
            if (reverse_handler) {
              reverse_handler->HandlePacket(packet, send_packet);
            }
          }
        }

        connection.Shutdown();
        running = false;
        return;
      }

      while (running) {
        if (!connection.ReadPacket(&packet)) {
          if (DebugHandshake()) {
            std::cerr << "[handshake] client_from_server read_failed\n" << std::flush;
          }
          Sleep(5);
          continue;
        }
        last_rx_ms.store(NowMs());
        if (DebugHandshake()) {
          std::cerr << "[handshake] client_from_server header="
                    << static_cast<int>(packet.header())
                    << " bytes=" << packet.payload().size() << "\n" << std::flush;
        }
        if (packet.header() == static_cast<uint8_t>(ut::KEEP_ALIVE)) {
          continue;
        }
        if (packet.header() == static_cast<uint8_t>(ut::TERMINAL_BUFFER)) {
          if (!enable_terminal_output) {
            continue;
          }
          ut::TerminalBuffer tb;
          if (!tb.ParseFromString(packet.payload())) {
            continue;
          }
          std::string output = tb.buffer();
          predictor.OnRemoteOutput(&output);
          if (output.empty()) {
            continue;
          }
          DWORD written = 0;
          WriteFile(stdout_handle, output.data(), static_cast<DWORD>(output.size()), &written, nullptr);
          if (DebugHandshake()) {
            std::cerr << "[handshake] client_from_server write bytes=" << written << "\n" << std::flush;
          }
        } else if (packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DESTINATION_REQUEST)) {
          if (reverse_handler) {
            reverse_handler->HandlePacket(packet, send_packet);
          }
        } else if (packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DESTINATION_RESPONSE) ||
                   packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DATA)) {
          if (forward_handler) {
            forward_handler->HandlePacket(packet, send_packet);
          }
          if (reverse_handler) {
            reverse_handler->HandlePacket(packet, send_packet);
          }
        }
      }
      running = false;
    });

    std::thread forward_thread;
    if (forward_handler || reverse_handler) {
      forward_thread = std::thread([&]() {
        while (running) {
          if (forward_handler) {
            forward_handler->Update(send_packet);
          }
          if (reverse_handler) {
            reverse_handler->Update(send_packet);
          }
          Sleep(10);
        }
      });
    }

    std::thread resize_thread;
    if (interactive) {
      resize_thread = std::thread([&]() {
        COORD last = GetConsoleSize();
        SendTerminalInfo(connection, client_id, last.X, last.Y);
        while (running) {
          Sleep(200);
          COORD current = GetConsoleSize();
          if (current.X != last.X || current.Y != last.Y) {
            SendTerminalInfo(connection, client_id, current.X, current.Y);
            last = current;
          }
        }
      });
    }

    if (input_thread.joinable()) {
      input_thread.join();
    }
    if (output_thread.joinable()) {
      output_thread.join();
    }
    running = false;
    if (resize_thread.joinable()) {
      resize_thread.join();
    }
    if (forward_thread.joinable()) {
      forward_thread.join();
    }

    if (keepalive_thread.joinable()) {
      keepalive_thread.join();
    }
    return 0;
  }

  if (argc > 3 && std::string(argv[1]) == "--connect") {
    std::string host = argv[2];
    int port = std::stoi(argv[3]);
    std::string client_id = argc > 4 ? argv[4] : ClientId::GetOrCreate();
    std::string passkey;
    std::string tunnel_arg;
    std::string reverse_tunnel_arg;
    std::string jumphost_arg;
    int jport_arg = 2022;
    std::string command_arg;
    bool noexit = false;
    bool tunnel_only = false;
    bool predictive_echo = false;
    if (DebugHandshake()) {
      std::cerr << "[handshake] client_connect_mode start\n";
    }

    for (int i = 4; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--key" && i + 1 < argc) {
        passkey = argv[++i];
        continue;
      }
      if ((arg == "--jumphost" || arg == "-jumphost") && i + 1 < argc) {
        jumphost_arg = argv[++i];
        continue;
      }
      if ((arg == "--jport" || arg == "-jport") && i + 1 < argc) {
        jport_arg = std::stoi(argv[++i]);
        continue;
      }
      if ((arg == "-t" || arg == "--tunnel") && i + 1 < argc) {
        tunnel_arg = argv[++i];
        continue;
      }
      if ((arg == "-r" || arg == "--reversetunnel") && i + 1 < argc) {
        reverse_tunnel_arg = argv[++i];
        continue;
      }
      if ((arg == "-c" || arg == "--command") && i + 1 < argc) {
        command_arg = argv[++i];
        continue;
      }
      if (arg == "--predictive-echo") {
        predictive_echo = true;
        continue;
      }
      if (arg == "--noexit") {
        noexit = true;
        continue;
      }
      if (arg == "--tunnel-only") {
        tunnel_only = true;
        continue;
      }
    }
    if (passkey.empty()) {
      std::cerr << "Missing --key passkey\n";
      return 1;
    }
    if (tunnel_only && !command_arg.empty()) {
      std::cerr << "--tunnel-only cannot be combined with --command\n";
      return 1;
    }

    WinsockContext winsock;
    (void)winsock;

    ut::SocketEndpoint endpoint;
    if (!jumphost_arg.empty()) {
      endpoint.set_name(jumphost_arg);
      endpoint.set_port(jport_arg);
    } else {
      endpoint.set_name(host);
      endpoint.set_port(port);
    }

    const bool interactive = !tunnel_only && (command_arg.empty() || noexit);
    const bool enable_keepalive = interactive || tunnel_only;
    const bool enable_terminal_output = !tunnel_only;

    auto socket_handler = std::make_shared<ut::TcpSocketHandler>();
    ut::ClientConnection connection(socket_handler, endpoint, client_id, passkey);
    connection.SetReconnectEnabled(interactive);
    if (!connection.Connect()) {
      std::cerr << "Failed to connect to server\n";
      return 1;
    }

    const bool returning_client = connection.IsReturningClient();
    if (!returning_client) {
      ut::InitialPayload payload;
      if (!jumphost_arg.empty()) {
        payload.set_jumphost(true);
        (*payload.mutable_environmentvariables())["dsthost"] = host;
        (*payload.mutable_environmentvariables())["dstport"] = std::to_string(port);
      } else {
        payload.set_jumphost(false);
      }
      if (!reverse_tunnel_arg.empty()) {
        try {
          for (const auto& req : ut::ParseRangesToRequests(reverse_tunnel_arg)) {
            *payload.add_reversetunnels() = req;
          }
        } catch (const std::exception& ex) {
          std::cerr << "Reverse tunnel parse failed: " << ex.what() << "\n";
          return 1;
        }
      }
      std::string payload_bytes;
      payload.SerializeToString(&payload_bytes);
      connection.WritePacket(ut::Packet(static_cast<uint8_t>(ut::INITIAL_PAYLOAD), payload_bytes));

      ut::Packet response_packet;
      if (!connection.ReadPacket(&response_packet) || response_packet.header() != static_cast<uint8_t>(ut::INITIAL_RESPONSE)) {
        std::cerr << "Missing initial response\n";
        return 1;
      }
      ut::InitialResponse response;
      if (!response.ParseFromString(response_packet.payload()) || !response.error().empty()) {
        std::cerr << "Initial response error: " << response.error() << "\n";
        return 1;
      }

      if (!command_arg.empty()) {
        ut::TerminalBuffer tb;
        tb.set_buffer(NormalizeCommand(command_arg));
        std::string tb_bytes;
        if (tb.SerializeToString(&tb_bytes)) {
          connection.WritePacket(ut::Packet(static_cast<uint8_t>(ut::TERMINAL_BUFFER), tb_bytes));
        }
      }
      if (!noexit && !command_arg.empty()) {
        ut::TerminalBuffer tb;
        tb.set_buffer("exit\r\n");
        std::string tb_bytes;
        if (tb.SerializeToString(&tb_bytes)) {
          connection.WritePacket(ut::Packet(static_cast<uint8_t>(ut::TERMINAL_BUFFER), tb_bytes));
        }
      }
    }

    PseudoTerminalConsole console;
    if (interactive) {
      console.EnableVirtualTerminal();
      console.EnableRawInput();
    }

    HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    PredictiveEcho predictor(predictive_echo && interactive, stdout_handle);
    std::atomic<bool> running{true};
    std::atomic<int64_t> last_rx_ms{NowMs()};

    constexpr int64_t kKeepaliveIntervalMs = 5000;
    constexpr int64_t kKeepaliveDeadMs = 15000;
    std::thread keepalive_thread;
    if (enable_keepalive) {
      keepalive_thread = std::thread([&]() {
        while (running) {
          Sleep(static_cast<DWORD>(kKeepaliveIntervalMs));
          if (!running) {
            break;
          }
          connection.Write(ut::Packet(static_cast<uint8_t>(ut::KEEP_ALIVE), ""));
          const int64_t age = NowMs() - last_rx_ms.load();
          if (age > kKeepaliveDeadMs) {
            connection.CloseSocketAndMaybeReconnect();
            last_rx_ms.store(NowMs());
          }
        }
      });
    }

    std::vector<ut::PortForwardSourceRequest> forward_requests;
    if (!tunnel_arg.empty()) {
      for (const auto& req : ut::ParseRangesToRequests(tunnel_arg)) {
        forward_requests.push_back(req);
      }
    }

    std::shared_ptr<ut::PortForwardHandler> forward_handler;
    if (!forward_requests.empty()) {
      forward_handler = std::make_shared<ut::PortForwardHandler>(socket_handler, false);
      try {
        for (const auto& req : forward_requests) {
          forward_handler->AddForwardRequest(req);
        }
      } catch (const std::exception& ex) {
        std::cerr << "Tunnel parse failed: " << ex.what() << "\n";
        return 1;
      }
    }
    std::shared_ptr<ut::PortForwardHandler> reverse_handler;
    if (!reverse_tunnel_arg.empty()) {
      reverse_handler = std::make_shared<ut::PortForwardHandler>(socket_handler, true);
    }
    auto send_packet = [&](const ut::Packet& packet) { connection.WritePacket(packet); };

    std::thread input_thread;
    if (interactive) {
      input_thread = std::thread([&]() {
        std::vector<char> buffer(4096);
        DWORD read_bytes = 0;
        while (running && ReadFile(stdin_handle, buffer.data(), static_cast<DWORD>(buffer.size()), &read_bytes, nullptr)) {
          if (read_bytes == 0) {
            break;
          }
          predictor.OnLocalInput(buffer.data(), static_cast<size_t>(read_bytes));
          ut::TerminalBuffer tb;
          tb.set_buffer(std::string(buffer.data(), buffer.data() + read_bytes));
          std::string tb_bytes;
          if (!tb.SerializeToString(&tb_bytes)) {
            continue;
          }
          connection.WritePacket(ut::Packet(static_cast<uint8_t>(ut::TERMINAL_BUFFER), tb_bytes));
        }
        running = false;
      });
    }

    std::thread output_thread([&]() {
      ut::Packet packet;
      if (!interactive && !tunnel_only) {
        const int64_t start_ms = NowMs();
        int64_t last_output_ms = start_ms;
        bool saw_output = false;
        constexpr int64_t kFirstOutputTimeoutMs = 5000;
        constexpr int64_t kIdleExitMs = 500;

        while (running) {
          auto reader = connection.reader();
          if (!reader || !reader->HasData()) {
            const int64_t now = NowMs();
            if (!saw_output && now - start_ms > kFirstOutputTimeoutMs) {
              break;
            }
            if (saw_output && now - last_output_ms > kIdleExitMs) {
              break;
            }
            Sleep(5);
            continue;
          }

          if (!connection.ReadPacket(&packet)) {
            Sleep(5);
            continue;
          }
          if (packet.header() == static_cast<uint8_t>(ut::KEEP_ALIVE)) {
            continue;
          }
          if (packet.header() == static_cast<uint8_t>(ut::TERMINAL_BUFFER)) {
            if (!enable_terminal_output) {
              continue;
            }
            ut::TerminalBuffer tb;
            if (!tb.ParseFromString(packet.payload())) {
              continue;
            }
            std::string output = tb.buffer();
            predictor.OnRemoteOutput(&output);
            if (output.empty()) {
              continue;
            }
            DWORD written = 0;
            WriteFile(stdout_handle,
                      output.data(),
                      static_cast<DWORD>(output.size()),
                      &written,
                      nullptr);
            saw_output = true;
            last_output_ms = NowMs();
          } else if (packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DESTINATION_REQUEST)) {
            if (reverse_handler) {
              reverse_handler->HandlePacket(packet, send_packet);
            }
          } else if (packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DESTINATION_RESPONSE) ||
                     packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DATA)) {
            if (forward_handler) {
              forward_handler->HandlePacket(packet, send_packet);
            }
            if (reverse_handler) {
              reverse_handler->HandlePacket(packet, send_packet);
            }
          }
        }

        connection.Shutdown();
        running = false;
        return;
      }

      while (running) {
        if (!connection.ReadPacket(&packet)) {
          if (DebugHandshake()) {
            std::cerr << "[handshake] client_from_server read_failed\n" << std::flush;
          }
          Sleep(5);
          continue;
        }
        last_rx_ms.store(NowMs());
        if (DebugHandshake()) {
          std::cerr << "[handshake] client_from_server header="
                    << static_cast<int>(packet.header())
                    << " bytes=" << packet.payload().size() << "\n" << std::flush;
        }
        if (packet.header() == static_cast<uint8_t>(ut::KEEP_ALIVE)) {
          continue;
        }
        if (packet.header() == static_cast<uint8_t>(ut::TERMINAL_BUFFER)) {
          if (!enable_terminal_output) {
            continue;
          }
          ut::TerminalBuffer tb;
          if (!tb.ParseFromString(packet.payload())) {
            continue;
          }
          std::string output = tb.buffer();
          predictor.OnRemoteOutput(&output);
          if (output.empty()) {
            continue;
          }
          DWORD written = 0;
          WriteFile(stdout_handle, output.data(), static_cast<DWORD>(output.size()), &written, nullptr);
          if (DebugHandshake()) {
            std::cerr << "[handshake] client_from_server write bytes=" << written << "\n" << std::flush;
          }
        } else if (packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DESTINATION_REQUEST)) {
          if (reverse_handler) {
            reverse_handler->HandlePacket(packet, send_packet);
          }
        } else if (packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DESTINATION_RESPONSE) ||
                   packet.header() == static_cast<uint8_t>(ut::PORT_FORWARD_DATA)) {
          if (forward_handler) {
            forward_handler->HandlePacket(packet, send_packet);
          }
          if (reverse_handler) {
            reverse_handler->HandlePacket(packet, send_packet);
          }
        }
      }
      running = false;
    });

    std::thread forward_thread;
    if (forward_handler || reverse_handler) {
      forward_thread = std::thread([&]() {
        while (running) {
          if (forward_handler) {
            forward_handler->Update(send_packet);
          }
          if (reverse_handler) {
            reverse_handler->Update(send_packet);
          }
          Sleep(10);
        }
      });
    }

    std::thread resize_thread;
    if (interactive) {
      resize_thread = std::thread([&]() {
        COORD last = GetConsoleSize();
        SendTerminalInfo(connection, client_id, last.X, last.Y);
        while (running) {
          Sleep(200);
          COORD current = GetConsoleSize();
          if (current.X != last.X || current.Y != last.Y) {
            SendTerminalInfo(connection, client_id, current.X, current.Y);
            last = current;
          }
        }
      });
    }

    if (input_thread.joinable()) {
      input_thread.join();
    }
    if (output_thread.joinable()) {
      output_thread.join();
    }
    running = false;
    if (resize_thread.joinable()) {
      resize_thread.join();
    }
    if (forward_thread.joinable()) {
      forward_thread.join();
    }

    if (keepalive_thread.joinable()) {
      keepalive_thread.join();
    }

    return 0;
  }

  std::cout << "Undying Terminal client stub (undying-terminal)\n";
  return 0;
}
