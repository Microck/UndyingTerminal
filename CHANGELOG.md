# Changelog

All notable changes to Undying Terminal will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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

[1.0.1]: https://github.com/Microck/UndyingTerminal/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/Microck/UndyingTerminal/releases/tag/v1.0.0
