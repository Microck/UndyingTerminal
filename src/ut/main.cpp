#include <atomic>
#include <chrono>
#include <exception>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "ClientId.hpp"
#include "PseudoTerminalConsole.hpp"
#include "SshConfig.hpp"
#include "SshCommandBuilder.hpp"
#include "SshSubprocess.hpp"
#include "WinsockContext.hpp"

#include "et/ClientConnection.hpp"
#include "et/Packet.hpp"
#include "et/PortForwardHandler.hpp"
#include "et/TcpSocketHandler.hpp"
#include "et/TunnelUtils.hpp"
#include "EtConstants.hpp"
#include "ET.pb.h"
#include "ETerminal.pb.h"

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
  et::TerminalInfo info;
  info.set_id(client_id);
  info.set_width(cols);
  info.set_height(rows);
  std::string payload;
  if (!info.SerializeToString(&payload)) {
    return false;
  }
  ut::Packet packet(static_cast<uint8_t>(et::TERMINAL_INFO), payload);
  connection.WritePacket(packet);
  return true;
}
}

int main(int argc, char** argv) {
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
      if (arg == "--noexit") {
        noexit = true;
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

    const std::string seed_id = "XXX" + GenerateRandom(13);
    const std::string seed_key = GenerateRandom(32);
    const char* term_env = std::getenv("TERM");
    const std::string term = term_env ? term_env : "xterm-256color";
    const std::string idpasskey = seed_id + "/" + seed_key + "_" + term;
    std::string remote_cmd = "echo '" + idpasskey + "' | " + remote_terminal;

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

    et::SocketEndpoint endpoint;
    if (!jumphost_arg.empty()) {
      endpoint.set_name(jumphost_arg);
      endpoint.set_port(jport_arg);
    } else {
      endpoint.set_name(host);
      endpoint.set_port(server_port);
    }

    const bool interactive = command_arg.empty() || noexit;

    std::vector<et::PortForwardSourceRequest> forward_requests;
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
      et::InitialPayload payload;
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
      connection.WritePacket(ut::Packet(static_cast<uint8_t>(et::INITIAL_PAYLOAD), payload_bytes));

      ut::Packet response_packet;
      if (!connection.ReadPacket(&response_packet) || response_packet.header() != static_cast<uint8_t>(et::INITIAL_RESPONSE)) {
        std::cerr << "Missing initial response\n";
        return 1;
      }
      et::InitialResponse response;
      if (!response.ParseFromString(response_packet.payload()) || !response.error().empty()) {
        std::cerr << "Initial response error: " << response.error() << "\n";
        return 1;
      }

      if (!command_arg.empty()) {
        et::TerminalBuffer tb;
        tb.set_buffer(command_arg);
        std::string tb_bytes;
        if (tb.SerializeToString(&tb_bytes)) {
          connection.WritePacket(ut::Packet(static_cast<uint8_t>(et::TERMINAL_BUFFER), tb_bytes));
        }
      }
      if (!noexit && !command_arg.empty()) {
        et::TerminalBuffer tb;
        tb.set_buffer("exit\r\n");
        std::string tb_bytes;
        if (tb.SerializeToString(&tb_bytes)) {
          connection.WritePacket(ut::Packet(static_cast<uint8_t>(et::TERMINAL_BUFFER), tb_bytes));
        }
      }
    }
 
     PseudoTerminalConsole console;
     console.EnableVirtualTerminal();
     console.EnableRawInput();

    HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    std::atomic<bool> running{true};
    std::atomic<int64_t> last_rx_ms{NowMs()};

    constexpr int64_t kKeepaliveIntervalMs = 5000;
    constexpr int64_t kKeepaliveDeadMs = 15000;
    std::thread keepalive_thread;
    if (interactive) {
      keepalive_thread = std::thread([&]() {
        while (running) {
          Sleep(static_cast<DWORD>(kKeepaliveIntervalMs));
          if (!running) {
            break;
          }
          connection.Write(ut::Packet(static_cast<uint8_t>(et::KEEP_ALIVE), ""));
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
          et::TerminalBuffer tb;
          tb.set_buffer(std::string(buffer.data(), buffer.data() + read_bytes));
          std::string tb_bytes;
          if (!tb.SerializeToString(&tb_bytes)) {
            continue;
          }
          connection.WritePacket(ut::Packet(static_cast<uint8_t>(et::TERMINAL_BUFFER), tb_bytes));
        }
        running = false;
      });
    }

    std::thread output_thread([&]() {
      ut::Packet packet;
      if (!interactive) {
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
          if (packet.header() == static_cast<uint8_t>(et::KEEP_ALIVE)) {
            continue;
          }
          if (packet.header() != static_cast<uint8_t>(et::TERMINAL_BUFFER)) {
            continue;
          }
          et::TerminalBuffer tb;
          if (!tb.ParseFromString(packet.payload())) {
            continue;
          }
          DWORD written = 0;
          WriteFile(stdout_handle,
                    tb.buffer().data(),
                    static_cast<DWORD>(tb.buffer().size()),
                    &written,
                    nullptr);
          saw_output = true;
          last_output_ms = NowMs();
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
        if (packet.header() == static_cast<uint8_t>(et::KEEP_ALIVE)) {
          continue;
        }
        if (packet.header() == static_cast<uint8_t>(et::TERMINAL_BUFFER)) {
          et::TerminalBuffer tb;
          if (!tb.ParseFromString(packet.payload())) {
            continue;
          }
          DWORD written = 0;
          WriteFile(stdout_handle, tb.buffer().data(), static_cast<DWORD>(tb.buffer().size()), &written, nullptr);
          if (DebugHandshake()) {
            std::cerr << "[handshake] client_from_server write bytes=" << written << "\n" << std::flush;
          }
        } else if (packet.header() == static_cast<uint8_t>(et::PORT_FORWARD_DESTINATION_REQUEST)) {
          if (reverse_handler) {
            reverse_handler->HandlePacket(packet, send_packet);
          }
        } else if (packet.header() == static_cast<uint8_t>(et::PORT_FORWARD_DESTINATION_RESPONSE) ||
                   packet.header() == static_cast<uint8_t>(et::PORT_FORWARD_DATA)) {
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
    if (interactive) {
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
    if (DebugHandshake()) {
      std::cerr << "[handshake] client_connect_mode start\n";
    }

    for (int i = 4; i + 1 < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--key") {
        passkey = argv[i + 1];
        continue;
      }
      if ((arg == "--jumphost" || arg == "-jumphost") && i + 1 < argc) {
        jumphost_arg = argv[i + 1];
        continue;
      }
      if ((arg == "--jport" || arg == "-jport") && i + 1 < argc) {
        jport_arg = std::stoi(argv[i + 1]);
        continue;
      }
      if ((arg == "-t" || arg == "--tunnel") && i + 1 < argc) {
        tunnel_arg = argv[i + 1];
        continue;
      }
      if ((arg == "-r" || arg == "--reversetunnel") && i + 1 < argc) {
        reverse_tunnel_arg = argv[i + 1];
        continue;
      }
      if ((arg == "-c" || arg == "--command") && i + 1 < argc) {
        command_arg = argv[i + 1];
        continue;
      }
      if (arg == "--noexit") {
        noexit = true;
        continue;
      }
    }
    if (passkey.empty()) {
      std::cerr << "Missing --key passkey\n";
      return 1;
    }

    WinsockContext winsock;
    (void)winsock;

    et::SocketEndpoint endpoint;
    if (!jumphost_arg.empty()) {
      endpoint.set_name(jumphost_arg);
      endpoint.set_port(jport_arg);
    } else {
      endpoint.set_name(host);
      endpoint.set_port(port);
    }

    const bool interactive = command_arg.empty() || noexit;

    auto socket_handler = std::make_shared<ut::TcpSocketHandler>();
    ut::ClientConnection connection(socket_handler, endpoint, client_id, passkey);
    connection.SetReconnectEnabled(interactive);
    if (!connection.Connect()) {
      std::cerr << "Failed to connect to server\n";
      return 1;
    }

    const bool returning_client = connection.IsReturningClient();
    if (!returning_client) {
      et::InitialPayload payload;
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
      connection.WritePacket(ut::Packet(static_cast<uint8_t>(et::INITIAL_PAYLOAD), payload_bytes));

      ut::Packet response_packet;
      if (!connection.ReadPacket(&response_packet) || response_packet.header() != static_cast<uint8_t>(et::INITIAL_RESPONSE)) {
        std::cerr << "Missing initial response\n";
        return 1;
      }
      et::InitialResponse response;
      if (!response.ParseFromString(response_packet.payload()) || !response.error().empty()) {
        std::cerr << "Initial response error: " << response.error() << "\n";
        return 1;
      }

      if (!command_arg.empty()) {
        et::TerminalBuffer tb;
        tb.set_buffer(command_arg);
        std::string tb_bytes;
        if (tb.SerializeToString(&tb_bytes)) {
          connection.WritePacket(ut::Packet(static_cast<uint8_t>(et::TERMINAL_BUFFER), tb_bytes));
        }
      }
      if (!noexit) {
        et::TerminalBuffer tb;
        tb.set_buffer("; exit\n");
        std::string tb_bytes;
        if (tb.SerializeToString(&tb_bytes)) {
          connection.WritePacket(ut::Packet(static_cast<uint8_t>(et::TERMINAL_BUFFER), tb_bytes));
        }
      }
    }

    PseudoTerminalConsole console;
    console.EnableVirtualTerminal();
    console.EnableRawInput();

    HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    std::atomic<bool> running{true};
    std::atomic<int64_t> last_rx_ms{NowMs()};

    constexpr int64_t kKeepaliveIntervalMs = 5000;
    constexpr int64_t kKeepaliveDeadMs = 15000;
    std::thread keepalive_thread([&]() {
      while (running) {
        Sleep(static_cast<DWORD>(kKeepaliveIntervalMs));
        if (!running) {
          break;
        }
        connection.Write(ut::Packet(static_cast<uint8_t>(et::KEEP_ALIVE), ""));
        const int64_t age = NowMs() - last_rx_ms.load();
        if (age > kKeepaliveDeadMs) {
          connection.CloseSocketAndMaybeReconnect();
          last_rx_ms.store(NowMs());
        }
      }
    });

    std::vector<et::PortForwardSourceRequest> forward_requests;
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
          et::TerminalBuffer tb;
          tb.set_buffer(std::string(buffer.data(), buffer.data() + read_bytes));
          std::string tb_bytes;
          if (!tb.SerializeToString(&tb_bytes)) {
            continue;
          }
          connection.WritePacket(ut::Packet(static_cast<uint8_t>(et::TERMINAL_BUFFER), tb_bytes));
        }
        running = false;
      });
    }

    std::thread output_thread([&]() {
      ut::Packet packet;
      if (!interactive) {
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
          if (packet.header() == static_cast<uint8_t>(et::KEEP_ALIVE)) {
            continue;
          }
          if (packet.header() != static_cast<uint8_t>(et::TERMINAL_BUFFER)) {
            continue;
          }
          et::TerminalBuffer tb;
          if (!tb.ParseFromString(packet.payload())) {
            continue;
          }
          DWORD written = 0;
          WriteFile(stdout_handle,
                    tb.buffer().data(),
                    static_cast<DWORD>(tb.buffer().size()),
                    &written,
                    nullptr);
          saw_output = true;
          last_output_ms = NowMs();
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
        if (packet.header() == static_cast<uint8_t>(et::KEEP_ALIVE)) {
          continue;
        }
        if (packet.header() == static_cast<uint8_t>(et::TERMINAL_BUFFER)) {
          et::TerminalBuffer tb;
          if (!tb.ParseFromString(packet.payload())) {
            continue;
          }
          DWORD written = 0;
          WriteFile(stdout_handle, tb.buffer().data(), static_cast<DWORD>(tb.buffer().size()), &written, nullptr);
          if (DebugHandshake()) {
            std::cerr << "[handshake] client_from_server write bytes=" << written << "\n" << std::flush;
          }
        } else if (packet.header() == static_cast<uint8_t>(et::PORT_FORWARD_DESTINATION_REQUEST)) {
          if (reverse_handler) {
            reverse_handler->HandlePacket(packet, send_packet);
          }
        } else if (packet.header() == static_cast<uint8_t>(et::PORT_FORWARD_DESTINATION_RESPONSE) ||
                   packet.header() == static_cast<uint8_t>(et::PORT_FORWARD_DATA)) {
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

    std::thread forward_thread([&]() {
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

    std::thread resize_thread([&]() {
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
