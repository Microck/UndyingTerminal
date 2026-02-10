# r/commandline Reddit Post

## title
my ssh sessions kept dying on windows, so i ported eternal terminal to c++

## body
i spend half my life in `ssh` + `tmux`. on linux, mosh and eternal terminal keep sessions alive when wifi drops or i close my laptop.

windows has always been painful. mosh scrollback is broken. et requires wsl. booting a full linux distro just to connect to a server felt wrong.

so i spent a few months porting et to native windows c++. it uses the windows pty apis (conpty) directly.

what it does:
- auto-reconnect. disconnect internet, drive home, session resumes.
- native `.exe`. no wsl needed.
- jump host support.
- scrollback that actually works.
- predictive echo for high-latency links.

it's mit licensed. pty handling on windows is weird. let me know if you hit edge cases.

repo: https://github.com/microck/undyingterminal
docs: https://undyingterminal.com/docs
site: https://undyingterminal.com

## tags for seo
eternal terminal, mosh alternative, windows terminal, persistent ssh, reconnectable shell, conpty, windows subsystem for linux

## posting tips
- post on weekday mornings for max visibility
- respond to every comment within first hour
- cross-post to r/cpp if technical discussion emerges
