# Blockers

## vcpkg on arm64-linux
The vcpkg manifest install fails on this Linux arm64 host during libsodium build.

Symptoms:
- `libsodium` fails to compile in `crypto_ipcrypt/ipcrypt_armcrypto.c` with Neon type errors.
- `x64-windows-static` triplet is not supported on this non-Windows host (vcpkg-msbuild not available).

Logs:
- `/home/ubuntu/workspace/undyingterminal/vcpkg/buildtrees/libsodium/build-arm64-linux-dbg-err.log`
- `/home/ubuntu/workspace/undyingterminal/build/vcpkg-manifest-install.log`

Resolution options:
1. Build on a Windows host using the `x64-windows-static` triplet.
2. Use a Linux-compatible triplet on this host and disable ARM crypto sources (patch or upstream fix).
3. For local Linux development, build with `UNDYING_TERMINAL_REQUIRE_DEPS=OFF` (deps stubbed).
