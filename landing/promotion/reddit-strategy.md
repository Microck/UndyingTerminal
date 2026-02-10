# Reddit Promotion Strategy for Undying Terminal

## Target Communities (in order of priority)

### 1. r/bashonubuntuonwindows (WSL Users)
**Why:** These users already know Eternal Terminal and feel the pain of WSL overhead.
**Best time to post:** Tuesday-Thursday, 9-11 AM EST
**Angle:** "Native alternative that doesn't need WSL"

### 2. r/PowerShell (Windows Admins)
**Why:** Direct target audience who deals with SSH on Windows daily.
**Best time to post:** Monday or Wednesday, 10 AM EST
**Angle:** "Solves VPN disconnect pain"
**Note:** Check for "Self-Promotion Saturday" threads if main post gets removed.

### 3. r/commandline (CLI Enthusiasts)
**Why:** Broad reach, technical audience.
**Best time to post:** Any weekday morning
**Angle:** "Fixed a problem I had"

### 4. r/cpp (C++ Developers)
**Why:** Engineering credibility, potential contributors.
**Best time to post:** Weekend (more technical discussion time)
**Angle:** "Here's how I solved these technical challenges"

## SEO Keywords to Include

Primary keywords (use in every post):
- eternal terminal
- windows terminal
- persistent ssh
- reconnectable shell
- mosh alternative

Secondary keywords (sprinkle in):
- windows conpty
- ssh session timeout
- vpn disconnect
- windows subsystem for linux (wsl)
- jump host
- port forwarding
- native windows

## Posting Rules Checklist

- [ ] Wait 9-12 hours between posts (don't spam)
- [ ] Respond to every comment in first 2 hours
- [ ] Don't use url shorteners
- [ ] Include GitHub link in body, not just title
- [ ] Check subreddit rules before posting
- [ ] Have 5-10 genuine comments in subreddit history first

## Cross-Posting Strategy

1. **Week 1:** Post to r/bashonubuntuonwindows (most receptive audience)
2. **Week 1:** Post to r/PowerShell (if WSL post gets good engagement)
3. **Week 2:** Post to r/commandline
4. **Week 2:** Post to r/cpp
5. **Week 3+:** Monitor for "ssh disconnect" threads and comment with solution

## Engagement Templates

### When someone asks "how is this different from tmux?"
> tmux keeps the session alive on the server, but your ssh connection still dies. undying terminal keeps the connection itself alive, so you don't need to re-attach. plus it has predictive echo for high-latency links.

### When someone says "just use mosh"
> mosh works but has broken scrollback on windows. this uses native windows conpty apis, so scrollback works properly and it integrates with windows terminal.

### When someone asks "why not just use wsl?"
> wsl works but has overhead. this is a single native exe with no vm boot time. it's not a replacement for wsl, just a lighter option if all you need is persistent ssh.

## Metrics to Track

- github stars increase
- release download count
- docs site traffic (check plausible)
- issues/discussions opened
- reddit post upvote ratio (aim for >70%)

## Backup Communities (if main posts do well)

- r/sysadmin (only post in "self-promotion saturday" threads)
- r/selfhosted (if you write about the server architecture)
- r/programming (only if post goes viral elsewhere first)
- r/sideproject (more casual, good for feedback)

## What NOT to Do

- Don't post to r/sysadmin main feed (will get removed)
- Don't post all at once (looks spammy)
- Don't ignore comments (hurts engagement algorithm)
- Don't use promotional language ("revolutionary", "game-changing")
- Don't argue in comments (stay helpful)

## Success Metrics

Good post:
- 50+ upvotes
- 10+ comments
- 5+ github stars from reddit traffic

Viral post:
- 200+ upvotes
- 50+ comments
- 20+ github stars
- mentioned in other threads
