# Changelog

All notable changes to Undying Terminal will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-02-08

### Added

- **Built-in Terminal UI** (`--ui`):
  - Interactive text-based UI for managing multiple sessions
  - Commands: `add`, `remove`, `list`, `start`, `stop`, `set-tunnel`, `set-flag`
  - Profile management with named configurations
  - Support for launching multiple concurrent sessions
  
- **Multi-Session / Tab Support**:
  - Named session profiles stored in memory
  - Concurrent running session processes
  - Independent session lifecycle management per profile
  
- **IPv6 Support**:
  - Full IPv6 addressing support in TCP client
  - Proper hostname resolution using `getaddrinfo()`
  - Tunnel parsing already supports bracketed IPv6 addresses
  
- **Mosh-like Predictive Echo** (`--predictive-echo`):
  - Local echo for better responsiveness on high-latency connections
  - Server-output reconciliation to maintain consistency
  - Available in both direct connect and SSH bootstrap modes
  
- **Tmux Integration** (`--tmux`, `--tmux-session`):
  - Automatic reconnection to existing tmux sessions
  - Custom tmux session name support
  - Wraps remote terminal launch inside tmux
  
- **Tunnel-Only Mode** (`--tunnel-only`):
  - Port forwarding without terminal session overhead
  - Keeps port-forwarding active without ConPTY
  - Useful for pure tunnel/relay use cases
  
- **Improved Command Execution** (`-c`):
  - Command payload now auto-appends newline
  - Conditional exit handling with `--noexit` flag
  - Better script-friendly remote command execution

### Changed

- **Static linking for MinGW builds**:
  - Executables are now statically linked (no external DLL dependencies)
  - Reduced executable size by ~80% (from ~30MB to ~5-6MB)
  - Better portability across Windows systems

### Fixed

- **Process cleanup in UI mode**:
  - Initialize `PROCESS_INFORMATION` struct before use
  - Check process exit code before calling `TerminateProcess()`
  - Add error reporting when process termination fails
  - Prevent undefined behavior with uninitialized handles

## [1.0.1] - 2026-02-07

### Added

- **SSH config file support** (`~/.ssh/config`):
  - `HostName` - Resolve host aliases to real hostnames
  - `User` - Default SSH username
  - `Port` - Custom SSH port
  - `IdentityFile` - Path to SSH private key
  - `ProxyJump` - Jump through intermediate hosts
  - `LocalForward` - Port forwarding from config file
  - `ForwardAgent` - Enable SSH agent forwarding
- **New CLI options**:
  - `--ssh-config <PATH>` - Specify custom SSH config file
  - `--no-ssh-config` - Disable SSH config parsing
  - `-A` / `--ssh-agent` - Enable SSH agent forwarding
  - `--no-ssh-agent` - Disable SSH agent forwarding
- Unit tests for SSH config parsing (`tests/ssh_config_test.cpp`)

### Changed

- Renamed protocol namespace from `et` to `ut`
- Renamed proto files from `ET.proto`/`ETerminal.proto` to `UT.proto`/`UTerminal.proto`
- Moved protocol files from `src/ut/et/` to `src/ut/protocol/`
- Updated protobuf ignore rules in `.gitignore`

### Fixed

- Server now properly cleans up on pipe disconnect
- Improved pipe socket handler disconnect detection

## [1.0.0] - 2026-02-06

### Added

- Initial release

[1.1.0]: https://github.com/Microck/UndyingTerminal/compare/v1.0.1...v1.1.0
[1.0.1]: https://github.com/Microck/UndyingTerminal/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/Microck/UndyingTerminal/releases/tag/v1.0.0
