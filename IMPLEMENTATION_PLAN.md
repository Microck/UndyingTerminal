# Implementation Plan (ET Parity, Undying Terminal)

## Scope


done
- Jump host support (--jumphost)
- Reverse tunnels (-r/--reversetunnel)
- Run command (-c/--command + --noexit)
- Protocol keepalive packets (KEEP_ALIVE)

remaining
- SSH config parsing (ProxyJump/LocalForward)
- ssh-agent forwarding
- server cleanup on pipe disconnect

DONE in code: UT protocol (seq/catchup), -t forwarding, IPv6.

## Fixed Decisions
- Match ET protocol and behavior, keep Undying Terminal branding and binary names.
- Protocol: ET protobufs + EternalTCP framing and recovery.
- Encryption: libsodium secretbox, ET nonce handling, per-direction keys.
- CLI: add ET-compatible flags for listed features only.

## References (ET)
- Protocol: https://raw.githubusercontent.com/MisterTea/EternalTerminal/master/docs/protocol.md
- How it works: https://eternalterminal.dev/howitworks
- Client CLI: https://raw.githubusercontent.com/MisterTea/EternalTerminal/master/src/terminal/TerminalClientMain.cpp
- Tunnel parsing: https://raw.githubusercontent.com/MisterTea/EternalTerminal/master/src/base/TunnelUtils.cpp
- Protos: https://raw.githubusercontent.com/MisterTea/EternalTerminal/master/proto/ET.proto
         https://raw.githubusercontent.com/MisterTea/EternalTerminal/master/proto/ETerminal.proto

## UT Protocol (Seq + Catchup)

DONE. See `src/ut/et/Connection.cpp`, `src/ut/et/BackedReader.*`, `src/ut/et/BackedWriter.*`.

## Port Forwarding (-t)

DONE (forward only). See `src/ut/et/PortForwardHandler.*`, `src/ut/et/TunnelUtils.*`, `src/ut/main.cpp`.

## Reverse Tunnels (-r)

DONE.

## Jump Host (--jumphost)

DONE.

## IPv6 Support

DONE. See `src/ut/et/TcpSocketHandler.cpp` + `src/ut/et/TunnelUtils.cpp`.

## Questions
- none
