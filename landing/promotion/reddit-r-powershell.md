# r/PowerShell Reddit Post

## title
stop ssh sessions from dying when your vpn drops

## body
i've been frustrated for years by how fragile remote sessions are on windows. if my vpn flickers or my laptop sleeps, the shell dies.

i know `tmux` preserves state on the server. but i wanted the connection itself to survive. there's a linux tool called "eternal terminal" that does this, but it never had a native windows port.

i built one. it wraps `cmd.exe` or `powershell.exe` on the server (or connects to linux servers) and keeps the tcp connection alive through network changes.

the main benefit:
you run the client on windows. you can reboot your router, and the terminal window pauses. when internet returns, it resumes. no re-typing passwords.

features:
- native windows conpty integration (no wsl needed)
- ssh config parsing (`~/.ssh/config`)
- jump host support via `ProxyJump`
- agent forwarding (`-A` flag)
- port forwarding (local and reverse tunnels)

it's fully open source. if you manage unstable remote connections, i'd appreciate feedback on whether this helps your workflow.

repo: https://github.com/microck/undyingterminal
download: https://github.com/microck/undyingterminal/releases/latest

## tags for seo
powershell remoting, windows ssh, eternal terminal windows, persistent shell, vpn disconnect, ssh session timeout, windows terminal

## posting tips
- frame as solving admin pain, not self-promo
- mention specific use cases (azure, aws, on-prem)
- check if there's a "tool tuesday" or similar thread first
