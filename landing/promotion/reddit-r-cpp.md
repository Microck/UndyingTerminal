# r/cpp Reddit Post

## title
porting a linux terminal emulator to windows (c++, cmake, vcpkg)

## body
i recently finished porting "eternal terminal" (a mosh-like tool) to windows. the original code depended heavily on linux-specific pty apis, so bringing it to windows involved some engineering.

the technical challenges:

1. conpty integration. i replaced the backend with the windows pseudo console api to render modern terminal sequences on windows 10/11. pipe handling here was interesting.

2. dependency management. i used `vcpkg` for dependencies (protobuf, libsodium, abseil). it made cross-platform compilation much saner than manual linking.

3. socket recoverability. the core logic uses a custom tcp protocol that handles "roaming" (client ip changing) without dropping state. had to handle windows socket quirks.

4. named pipes. windows uses named pipes instead of unix domain sockets for local communication between server and terminal processes.

what it does:
- keeps ssh sessions alive through network drops
- auto-reconnects without losing scrollback
- supports jump hosts and port forwarding
- native `.exe`, no cygwin or msys2

it's mit licensed. if anyone wants to review the conpty implementation or has feedback on the cmake setup, i'd appreciate it.

repo: https://github.com/microck/undyingterminal

## tags for seo
windows conpty, c++ terminal emulator, cmake windows, vcpkg, named pipes, eternal terminal port

## posting tips
- focus on the engineering, not the marketing
- expect questions about conpty vs winpty
- be ready to discuss protobuf/sodium choices
- mention it's single-platform (windows) by design
