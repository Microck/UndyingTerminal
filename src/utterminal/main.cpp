#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "ConPTYSession.hpp"
#include "protocol/ClientConnection.hpp"
#include "protocol/PipeSocketHandler.hpp"
#include "protocol/Packet.hpp"
#include "protocol/TcpSocketHandler.hpp"
#include "UT.pb.h"
#include "UTerminal.pb.h"

#ifdef _WIN32
namespace {
bool DebugHandshake() {
  return std::getenv("UT_DEBUG_HANDSHAKE") != nullptr;
}

std::wstring GetPipeName() {
  const char* env = std::getenv("UT_PIPE_NAME");
  if (env && *env) {
    return std::wstring(env, env + std::strlen(env));
  }
  return L"\\\\.\\pipe\\undying-terminal";
}
}
int main(int argc, char** argv) {
  bool jump_mode = false;
  bool tunnel_only = false;
  std::wstring command = L"cmd.exe";
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--powershell") {
      command = L"powershell.exe";
    } else if (arg == "--jump") {
      jump_mode = true;
    } else if (arg == "--tunnel-only") {
      tunnel_only = true;
    }
  }

  std::string idpasskey_line;
  if (!std::getline(std::cin, idpasskey_line)) {
    std::cerr << "Missing id/passkey input\n";
    return 1;
  }

  std::string idpasskey = idpasskey_line;
  if (idpasskey.find('\n') != std::string::npos) {
    idpasskey.erase(idpasskey.find('\n'));
  }
  if (idpasskey.find('\r') != std::string::npos) {
    idpasskey.erase(idpasskey.find('\r'));
  }

  auto slash_pos = idpasskey.find('/');
  auto underscore_pos = idpasskey.find('_');
  if (slash_pos == std::string::npos) {
    std::cerr << "Invalid id/passkey format\n";
    return 1;
  }
  const std::string raw_id = idpasskey.substr(0, slash_pos);
  const std::string passkey = underscore_pos == std::string::npos
                                  ? idpasskey.substr(slash_pos + 1)
                                  : idpasskey.substr(slash_pos + 1, underscore_pos - slash_pos - 1);

  auto gen_random = [](size_t len) {
    static const char kChars[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, sizeof(kChars) - 2);
    std::string out;
    out.reserve(len);
    for (size_t i = 0; i < len; ++i) {
      out.push_back(kChars[dist(gen)]);
    }
    return out;
  };

  std::string client_id = raw_id;
  std::string passkey_final = passkey;
  if (raw_id.rfind("XXX", 0) == 0) {
    client_id = gen_random(16);
    passkey_final = gen_random(32);
    std::cout << client_id << "/" << passkey_final << "\n" << std::flush;
  }

  ut::PipeSocketHandler pipe_handler;
  const std::wstring pipe_name = GetPipeName();
  ut::SocketHandle pipe = pipe_handler.Connect(pipe_name);
  if (pipe == ut::kInvalidSocket) {
    std::cerr << "Failed to connect to named pipe\n";
    return 1;
  }

  ut::TerminalUserInfo tui;
  tui.set_id(client_id);
  tui.set_passkey(passkey_final);
  std::string tui_payload;
  if (!tui.SerializeToString(&tui_payload)) {
    std::cerr << "Failed to serialize TerminalUserInfo\n";
    return 1;
  }
  ut::Packet tui_packet(static_cast<uint8_t>(ut::TERMINAL_USER_INFO), tui_payload);
  pipe_handler.WritePacket(pipe, tui_packet);
  if (DebugHandshake()) {
    std::cerr << "[handshake] terminal_registered id_len=" << client_id.size()
              << " passkey_len=" << passkey_final.size() << "\n";
  }

  ut::Packet init_packet;
  if (!pipe_handler.ReadPacket(pipe, &init_packet)) {
    std::cerr << "Failed to read init packet\n";
    return 1;
  }
  if (init_packet.header() != static_cast<uint8_t>(jump_mode ? ut::JUMPHOST_INIT : ut::TERMINAL_INIT)) {
    std::cerr << "Unexpected init packet\n";
    return 1;
  }

  if (jump_mode) {
    ut::InitialPayload payload;
    if (!payload.ParseFromString(init_packet.payload())) {
      std::cerr << "Invalid jumphost init payload\n";
      return 1;
    }
    const auto& env = payload.environmentvariables();
    auto host_it = env.find("dsthost");
    auto port_it = env.find("dstport");
    if (host_it == env.end() || port_it == env.end()) {
      std::cerr << "Missing jumphost destination\n";
      return 1;
    }
    int dst_port = 0;
    try {
      dst_port = std::stoi(port_it->second);
    } catch (...) {
      std::cerr << "Invalid jumphost port\n";
      return 1;
    }

    auto socket_handler = std::make_shared<ut::TcpSocketHandler>();
    ut::SocketEndpoint endpoint;
    endpoint.set_name(host_it->second);
    endpoint.set_port(dst_port);

    ut::ClientConnection dest_connection(socket_handler, endpoint, client_id, passkey_final);
    if (!dest_connection.Connect()) {
      std::cerr << "Failed to connect to destination server\n";
      return 1;
    }

    payload.set_jumphost(false);
    std::string payload_bytes;
    payload.SerializeToString(&payload_bytes);
    dest_connection.WritePacket(ut::Packet(static_cast<uint8_t>(ut::INITIAL_PAYLOAD), payload_bytes));

    ut::Packet response_packet;
    if (!dest_connection.ReadPacket(&response_packet) ||
        response_packet.header() != static_cast<uint8_t>(ut::INITIAL_RESPONSE)) {
      std::cerr << "Missing destination initial response\n";
      return 1;
    }
    ut::InitialResponse response;
    if (!response.ParseFromString(response_packet.payload()) || !response.error().empty()) {
      std::cerr << "Destination initial response error: " << response.error() << "\n";
      return 1;
    }

    std::atomic<bool> running{true};
    std::thread pipe_to_dest([&]() {
      ut::Packet packet;
      while (running && pipe_handler.ReadPacket(pipe, &packet)) {
        if (DebugHandshake()) {
          std::cerr << "[handshake] jump pipe_to_dest header="
                    << static_cast<int>(packet.header())
                    << " bytes=" << packet.payload().size() << "\n";
        }
        dest_connection.WritePacket(packet);
        if (DebugHandshake()) {
          std::cerr << "[handshake] jump pipe_to_dest sent\n";
        }
      }
      running = false;
    });

    std::thread dest_to_pipe([&]() {
      ut::Packet packet;
      while (running) {
        if (!dest_connection.ReadPacket(&packet)) {
          if (DebugHandshake()) {
            std::cerr << "[handshake] jump dest_to_pipe read_failed\n";
          }
          Sleep(5);
          continue;
        }
        if (DebugHandshake()) {
          std::cerr << "[handshake] jump dest_to_pipe header="
                    << static_cast<int>(packet.header())
                    << " bytes=" << packet.payload().size() << "\n";
        }
        pipe_handler.WritePacket(pipe, packet);
        if (DebugHandshake()) {
          std::cerr << "[handshake] jump dest_to_pipe sent\n";
        }
      }
      running = false;
    });

    if (pipe_to_dest.joinable()) {
      pipe_to_dest.join();
    }
    if (dest_to_pipe.joinable()) {
      dest_to_pipe.join();
    }
    pipe_handler.Close(pipe);
    return 0;
  }

  if (tunnel_only) {
    ut::Packet packet;
    while (pipe_handler.ReadPacket(pipe, &packet)) {
      if (DebugHandshake()) {
        std::cerr << "[handshake] tunnel_only pipe_packet header="
                  << static_cast<int>(packet.header())
                  << " bytes=" << packet.payload().size() << "\n";
      }
    }
    pipe_handler.Close(pipe);
    return 0;
  }

  ConPTYSession session;
  if (!session.Start(command, false)) {
    std::cerr << "Failed to start ConPTY session\n";
    return 1;
  }

  std::thread input_thread([&]() {
    ut::Packet packet;
    while (session.IsRunning() && pipe_handler.ReadPacket(pipe, &packet)) {
      if (packet.header() == static_cast<uint8_t>(ut::TERMINAL_BUFFER)) {
        ut::TerminalBuffer tb;
        if (!tb.ParseFromString(packet.payload())) {
          continue;
        }
        if (DebugHandshake() && !jump_mode) {
          std::cerr << "[handshake] term input bytes=" << tb.buffer().size() << "\n";
        }
        DWORD written = 0;
        WriteFile(session.InputWriteHandle(), tb.buffer().data(), static_cast<DWORD>(tb.buffer().size()), &written, nullptr);
      } else if (packet.header() == static_cast<uint8_t>(ut::TERMINAL_INFO)) {
        ut::TerminalInfo info;
        if (!info.ParseFromString(packet.payload())) {
          continue;
        }
        if (info.width() > 0 && info.height() > 0) {
          session.Resize(static_cast<short>(info.width()), static_cast<short>(info.height()));
        }
      }
    }
  });

  std::thread output_thread([&]() {
    std::vector<char> buffer(4096);
    DWORD read_bytes = 0;
    while (session.IsRunning() && ReadFile(session.OutputReadHandle(), buffer.data(), static_cast<DWORD>(buffer.size()), &read_bytes, nullptr)) {
      if (read_bytes == 0) {
        break;
      }
      ut::TerminalBuffer tb;
      tb.set_buffer(std::string(buffer.data(), buffer.data() + read_bytes));
      if (DebugHandshake() && !jump_mode) {
        std::cerr << "[handshake] term output bytes=" << read_bytes << "\n";
      }
      std::string payload;
      if (!tb.SerializeToString(&payload)) {
        continue;
      }
      ut::Packet packet(static_cast<uint8_t>(ut::TERMINAL_BUFFER), payload);
      pipe_handler.WritePacket(pipe, packet);
    }
  });

  session.Wait();

  if (input_thread.joinable()) {
    input_thread.join();
  }
  if (output_thread.joinable()) {
    output_thread.join();
  }
  pipe_handler.Close(pipe);
  return 0;
}
#else
int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  std::cout << "undying-terminal-terminal is supported on Windows 10 1809+" << std::endl;
  return 0;
}
#endif
