# Contributing to Undying Terminal

Thank you for your interest in contributing to Undying Terminal! This document provides guidelines and instructions for contributing.

## Code of Conduct

By participating in this project, you agree to abide by our [Code of Conduct](CODE_OF_CONDUCT.md).

## How to Contribute

### Reporting Bugs

Before submitting a bug report:

1. Check the [FAQ](https://undyingterminal.mintlify.app/troubleshooting/faq) for common issues
2. Search [existing issues](https://github.com/Microck/UndyingTerminal/issues) to avoid duplicates
3. Enable debug logging (`$env:UT_DEBUG_HANDSHAKE=1`) to gather more information

When reporting a bug, include:

- Windows version (`[System.Environment]::OSVersion.Version`)
- Undying Terminal version (`undying-terminal.exe --version`)
- Steps to reproduce
- Expected vs actual behavior
- Full error messages and debug output

### Suggesting Features

Feature requests are welcome! Please:

1. Search existing issues to avoid duplicates
2. Clearly describe the use case
3. Explain why this would benefit other users

### Pull Requests

#### Getting Started

1. Fork the repository
2. Clone your fork:
   ```powershell
   git clone https://github.com/YOUR_USERNAME/UndyingTerminal.git
   cd UndyingTerminal
   ```

3. Set up the build environment:
   ```powershell
   .\vcpkg\bootstrap-vcpkg.bat
   cmake --preset windows-vcpkg-static
   ```

4. Create a feature branch:
   ```powershell
   git checkout -b feature/your-feature-name
   ```

#### Development Workflow

1. Make your changes
2. Build and test:
   ```powershell
   cmake --build --preset windows-vcpkg-static
   ctest --preset windows-vcpkg-static
   ```

3. Ensure code follows existing style conventions
4. Update documentation if needed
5. Commit with clear, descriptive messages

#### Commit Messages

Use clear, descriptive commit messages:

```
feat: add SSH config parsing for ProxyJump directive

- Parse ProxyJump from ~/.ssh/config
- Support chained jump hosts
- Add unit tests for edge cases
```

Prefix with:
- `feat:` - New features
- `fix:` - Bug fixes
- `docs:` - Documentation changes
- `refactor:` - Code refactoring
- `test:` - Adding/updating tests
- `build:` - Build system changes
- `chore:` - Maintenance tasks

#### Submitting

1. Push your branch:
   ```powershell
   git push origin feature/your-feature-name
   ```

2. Open a Pull Request against `main`
3. Fill out the PR template
4. Wait for review

## Development Setup

### Prerequisites

- Windows 10 Build 17763+ or Windows 11
- Visual Studio 2019+ with C++ Desktop Development workload
- CMake 3.20+
- Ninja build system

### Building

```powershell
# Bootstrap vcpkg
.\vcpkg\bootstrap-vcpkg.bat

# Configure
cmake --preset windows-vcpkg-static

# Build
cmake --build --preset windows-vcpkg-static

# Run tests
ctest --preset windows-vcpkg-static
```

### Project Structure

```
UndyingTerminal/
|-- src/
|   |-- ut/              # Client executable
|   |   |-- protocol/    # Network protocol implementation
|   |   |-- SshConfig.*  # SSH config parsing
|   |   `-- main.cpp     # Client entry point
|   |-- utserver/        # Server executable
|   `-- utterminal/      # Terminal executable
|-- proto/               # Protocol buffer definitions
|-- build/               # Generated protobuf files
|-- tests/               # Unit tests
|-- docs/                # Documentation (Mintlify)
`-- vcpkg/               # Package manager
```

### Running Tests

```powershell
# Run all tests
ctest --preset windows-vcpkg-static

# Run specific test
ctest --preset windows-vcpkg-static -R ssh_config
```

## Style Guidelines

### C++ Code Style

- Use modern C++ (C++17)
- Follow existing code conventions in the repository
- Use meaningful variable and function names
- Add comments for complex logic
- Keep functions focused and reasonably sized

### Documentation

- Update relevant `.mdx` files in `docs/` for user-facing changes
- Update `README.md` for significant features
- Add inline code comments for complex logic

## Questions?

- Check the [documentation](https://undyingterminal.mintlify.app/)
- File an [issue](https://github.com/Microck/UndyingTerminal/issues)

Thank you for contributing!
