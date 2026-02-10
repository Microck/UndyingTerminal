# Pending Features: Eternal Terminal vs Undying Terminal

This document lists features from Eternal Terminal that Undying Terminal does not yet implement.

## Core Features

### 1. Built-in Terminal UI
- **Eternal Terminal**: Includes `etclient` with a built-in text-based UI
- **Undying Terminal**: Uses separate binaries (`server.exe`, `terminal.exe`, `client.exe`)
- **Value**: Simpler deployment and usage with a single executable
- **Status**: Implemented (new built-in text UI via `undying-terminal --ui` with profile management and session launch controls)

### 2. Multi-Session / Tab Support
- **Eternal Terminal**: Native support for multiple sessions/tabs within one client instance
- **Undying Terminal**: Single-session focused architecture
- **Value**: Users can manage multiple shells without launching multiple clients
- **Status**: Implemented (built-in UI supports multiple named session profiles and concurrent running session processes)

### 3. IPv6 Support
- **Eternal Terminal**: Full IPv6 addressing support
- **Undying Terminal**: IPv4 only
- **Value**: Future-proof networking for IPv6-only environments
- **Status**: Implemented (TCP client now resolves IPv6 hosts; tunnel parsing already handles bracketed IPv6)

### 4. Mosh-like Predictive Echo
- **Eternal Terminal**: Implements predictive local echo for better responsiveness on high-latency connections
- **Undying Terminal**: No local echo prediction
- **Value**: Instant feedback when typing over slow/high-latency links
- **Status**: Implemented (optional `--predictive-echo` performs local echo with server-output reconciliation)

### 5. Tmux Integration
- **Eternal Terminal**: Can automatically reconnect to existing tmux sessions
- **Undying Terminal**: No tmux awareness
- **Value**: Seamless integration with existing terminal multiplexer workflows
- **Status**: Implemented (SSH mode supports `--tmux` and `--tmux-session` to wrap remote terminal launch inside tmux)

### 6. IRC-style Command Execution
- **Eternal Terminal**: Supports `et -c "command" host` for one-shot commands
- **Undying Terminal**: Has `-c` flag but limited implementation
- **Value**: Script-friendly remote command execution
- **Status**: Implemented (command payload now auto-appends newline; exit handling is conditional on `--noexit` and command presence)

## Protocol & Networking

### 7. Tunnel-Only Mode
- **Eternal Terminal**: `-t` flag for tunnel-only connections without terminal
- **Undying Terminal**: Tunnels require active terminal session
- **Value**: Use ET purely as a tunnel/relay without shell overhead
- **Status**: Implemented via `--tunnel-only` (keeps port-forwarding active without ConPTY; `-t` remains the tunnel-argument flag)

## Future Considerations

The following features from Eternal Terminal are **not implemented** but could be considered for future releases based on user demand:

### 8. Tmux Control Mode (`tmux -CC`)

- **Eternal Terminal**: Full `tmux -CC` control mode support with bidirectional protocol
- **Undying Terminal**: Basic tmux wrapping only (`--tmux`, `--tmux-session`)
- **Value**: GUI-like window management, programmatic control of tmux sessions, native tab integration
- **Status**: Not Implemented (complexity vs. value trade-off)
- **Rationale**: 
  - Requires implementing tmux control mode protocol parser (%begin, %end, %output markers)
  - Significant effort (~500+ lines, protocol state machine, async I/O)
  - Limited benefit on Windows (Windows Terminal already has native tabs)
  - Basic tmux wrapping provides 90% of the value (session persistence)
  - Would be valuable for users wanting advanced tmux GUI integration
- **Implementation Notes**:
  - Protocol: https://github.com/tmux/tmux/wiki/Control-Mode
  - Requires bidirectional text protocol handling
  - State machine for tracking windows/panes
  - UI abstraction layer for window management

## Notes

- Cross-platform support (Linux/macOS) was intentionally excluded per requirements
- Windows remains the primary target platform
- Priority should be based on user workflow impact
- All originally planned features (1-7) are now implemented as of v1.1.0

## References

- Eternal Terminal: https://eternalterminal.dev/
- GitHub: https://github.com/MisterTea/EternalTerminal
- Undying Terminal: https://github.com/Microck/UndyingTerminal
