<div align="center">
  <img src="assets/logo/UndyngTerminalLogo.svg" width="150" alt="undying terminal logo" />
  
  <h1>Undying Terminal</h1>
  
  <p><strong>reconnectable secure remote shell, on Windows</strong></p>

  <p>
    <a href="LICENSE">
      <img src="https://img.shields.io/badge/license-MIT-blue.svg?style=flat-square" alt="license" />
    </a>
    <img src="https://img.shields.io/badge/platform-windows-0078D6.svg?style=flat-square" alt="windows" />
    <img src="https://img.shields.io/badge/build-cmake-064F8C.svg?style=flat-square" alt="cmake" />
    <img src="https://img.shields.io/badge/lang-c%2B%2B-00599C.svg?style=flat-square" alt="c++" />
  </p>

  <br />
</div>

- docs: https://undyingterminal.mintlify.app/
- releases: https://github.com/Microck/UndyingTerminal/releases
- issues: https://github.com/Microck/UndyingTerminal/issues

## what

- you run a local server + local terminal.
- clients connect over tcp (direct or bootstrapped via ssh).
- session stays alive via keepalive + reconnect + recovery.
- supports forward tunnels and reverse tunnels.
- supports a jump host hop (client -> jump server -> destination).

## how it works

```mermaid
flowchart LR
  subgraph local_machine[local machine]
    T[undying-terminal-terminal.exe<br/>conpty + shell] <-- named pipe --> S[undying-terminal-server.exe<br/>session registry]
  end

  C[undying-terminal.exe<br/>client] <-- tcp + crypto --> S
  T --> SH[shell<br/>cmd.exe / powershell.exe]
```

## quick start (windows)

prereq
- use windows terminal or cmd/powershell.

run

```powershell
# 1) start server (listens on 2022 by default)
./undying-terminal-server.exe

# 2) start terminal (prints id/passkey once)
echo "XXX/ignored" | ./undying-terminal-terminal.exe

# 3) connect (interactive)
./undying-terminal.exe --connect 127.0.0.1 2022 <client_id> --key <passkey> --noexit
```

one-shot command

```powershell
# note: include newline for cmd.exe
./undying-terminal.exe --connect 127.0.0.1 2022 <client_id> --key <passkey> -c "echo hi`r`n"
```

## ssh bootstrap

this starts a remote terminal over ssh, then connects to the local server.

```powershell
./undying-terminal.exe --ssh <host> -l <user>
```

## jumphost

mental model
- client connects to jump server.
- jump server tells its local terminal to proxy to the destination.
- jump terminal connects to destination server and shuttles packets.

```mermaid
sequenceDiagram
  participant C as client (undying-terminal.exe)
  participant JS as jump server (undying-terminal-server.exe)
  participant JT as jump terminal (--jump)
  participant DS as dest server
  participant DT as dest terminal

  C->>JS: connect (client_id/passkey)
  JS->>JT: JUMPHOST_INIT (dsthost/dstport)
  JT->>DS: connect (client_id/passkey)
  DS->>DT: TERMINAL_INIT
  C->>JS: terminal_buffer
  JS->>JT: terminal_buffer
  JT->>DS: terminal_buffer
  DS->>JT: terminal_buffer (output)
  JT->>JS: terminal_buffer (output)
  JS->>C: terminal_buffer (output)
```

note
- you only need `UT_PIPE_NAME` when running multiple servers on one machine (dev).

## tunnels

forward tunnels
- `-t/--tunnel`: open local ports that forward through the session.

reverse tunnels
- `-r/--reversetunnel`: server listens; when hit, it requests the client to connect to a destination and shuttles data.

## config

file
- `%PROGRAMDATA%\UndyingTerminal\ut.cfg`

keys

```ini
port=2022
bind_ip=0.0.0.0
verbose=false
```

env
- `UT_PIPE_NAME` override named pipe path (dev / multi-server).
- `UT_DEBUG_HANDSHAKE=1` prints packet-level debug.

## limitations (still missing)

- ssh config parsing (proxyjump/localforward)
- ssh-agent forwarding
- server cleanup on pipe disconnect

## license

mit. see `LICENSE`.
