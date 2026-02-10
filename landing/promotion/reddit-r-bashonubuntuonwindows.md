# r/bashonubuntuonwindows Reddit Post

## title
native windows alternative to eternal terminal

## body
i've used eternal terminal on linux for years. on windows, i used to run it inside wsl to connect to servers. booting a full distro just for a persistent shell felt heavy.

i decided to port the core et protocol to native windows c++. it uses the windows conpty api (same backend as windows terminal) instead of the linux pty.

why i built this over wsl+et:
- instant start. single `.exe`, no vm boot time
- better integration. copy/paste works natively with windows terminal
- lighter. no vmmem overhead
- scrollback actually works (mosh's broken scrollback was always annoying)
- built-in ui for managing multiple sessions (`--ui` flag)

features:
- auto-reconnect through network changes
- tmux integration for remote hosts (`--tmux` flag)
- predictive echo for high-latency connections
- ipv6 support
- ssh bootstrap mode (starts server over ssh)

if you use wsl mainly for ssh persistence, this might be a lighter option.

repo: https://github.com/microck/undyingterminal
site: https://undyingterminal.com

## tags for seo
wsl alternative, eternal terminal windows, native windows ssh, windows terminal, conpty, persistent ssh, windows subsystem for linux

## posting tips
- emphasize "native" vs "wsl" angle
- wsl users are technical, lean into the engineering details
- mention it's complementary to wsl, not a replacement
