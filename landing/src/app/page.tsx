"use client";

import { useCallback, useEffect, useRef, useState } from "react";
import { gsap } from "gsap";
import { TextPlugin } from "gsap/TextPlugin";
import { GeistPixelSquare } from "geist/font/pixel";
import Threads from "@/components/backgrounds/Threads";
import Features from "@/components/features-1";
import FAQs from "@/components/faqs";
import FooterSection from "@/components/footer";
import { LightRays } from "@/components/ui/light-rays";
import DecryptedText from "@/components/ui/decrypted-text";
import { ShinyText } from "@/components/ui/shiny-text";
import { TerminalSkeleton } from "@/components/terminal-skeleton";
import SmoothDropdown from "@/components/smooth-dropdown";

gsap.registerPlugin(TextPlugin);

const TITLE_TEXT = "Your session survives.";

// ── Terminal simulation content ──
const TERMINAL_SEQUENCE = {
  initialCommand: "ssh deploy@prod-server",
  connectionOutput: ["Connecting to prod-server...", "Authenticated via key.", ""],
  sessionStart: "deploy@prod:~$ ",
  typedCommand: "npm run deploy --production",
  deployOutput: [
    "Building application...",
    "Bundling assets... done",
    "Uploading to CDN...",
  ],
  disconnectMessage: "CONNECTION LOST",
  reconnectMessages: ["Reconnecting...", "Session restored."],
  resumeOutput: ["Upload complete. 47 files.", "Deployment successful.", ""],
  finalPrompt: "deploy@prod:~$ ",
};

export default function Home() {
  const heroSectionRef = useRef<HTMLElement>(null);
  const navRef = useRef<HTMLElement>(null);
  const heroTitleRef = useRef<HTMLHeadingElement>(null);
  const heroUnderlineRef = useRef<HTMLDivElement>(null);
  const terminalContainerRef = useRef<HTMLDivElement>(null);

  const terminalBodyRef = useRef<HTMLDivElement>(null);
  const statusRef = useRef<HTMLSpanElement>(null);
  const statusTextRef = useRef<HTMLSpanElement>(null);
  const rttRef = useRef<HTMLDivElement>(null);
  const retriesRef = useRef<HTMLDivElement>(null);
  const bufferRef = useRef<HTMLDivElement>(null);
  const retryCountRef = useRef(0);
  const scanlineRef = useRef<HTMLDivElement>(null);
  const glitchContainerRef = useRef<HTMLDivElement>(null);
  const flashRef = useRef<HTMLDivElement>(null);

  const ctaRef = useRef<HTMLDivElement>(null);
  const versionRef = useRef<HTMLParagraphElement>(null);

  const [reducedMotion, setReducedMotion] = useState(() => {
    if (typeof window === "undefined") return false;
    return window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  });

  const [threadsActive, setThreadsActive] = useState(false);
  const [heroDecryptDone, setHeroDecryptDone] = useState(false);
  const [isTerminalLoading, setIsTerminalLoading] = useState(true);

  // Fail-safe: if decrypt effect doesn't fire (e.g., IO edge-cases), switch to shiny.
  useEffect(() => {
    if (heroDecryptDone) return;
    const t = setTimeout(() => {
      setHeroDecryptDone(true);
    }, 1600);
    return () => clearTimeout(t);
  }, [heroDecryptDone]);

  const handleHeroDecryptDone = useCallback(() => {
    setHeroDecryptDone(true);
  }, []);

  const setStatus = useCallback(
    (status: "connected" | "disconnected" | "reconnecting") => {
      if (!statusRef.current || !statusTextRef.current) return;
      statusRef.current.className = `status-dot ${status}`;
      statusTextRef.current.textContent =
        status === "connected"
          ? "Connected"
          : status === "disconnected"
            ? "Disconnected"
            : "Reconnecting...";

      if (status === "reconnecting") {
        retryCountRef.current += 1;
      }

      if (rttRef.current) {
        rttRef.current.textContent =
          status === "connected"
            ? "rtt: n/a"
            : status === "reconnecting"
              ? "rtt: negotiating"
              : "rtt: offline";
      }
      if (retriesRef.current) {
        retriesRef.current.textContent = `retries: ${retryCountRef.current}`;
      }
      if (bufferRef.current) {
        bufferRef.current.textContent =
          status === "reconnecting" ? "buffer: replaying" : "buffer: persisted";
      }
    },
    [],
  );

  const typeText = useCallback(
    (element: HTMLElement, text: string, speed: number = 0.03): gsap.core.Tween => {
      return gsap.to(element, {
        duration: text.length * speed,
        text: { value: text, delimiter: "" },
        ease: "none",
      });
    },
    [],
  );

  useEffect(() => {
    const mql = window.matchMedia("(prefers-reduced-motion: reduce)");
    const onChange = () => setReducedMotion(mql.matches);
    onChange();
    mql.addEventListener("change", onChange);
    return () => mql.removeEventListener("change", onChange);
  }, []);

  // Simulate terminal loading state
  useEffect(() => {
    if (reducedMotion) {
      setIsTerminalLoading(false);
      return;
    }
    const timer = setTimeout(() => {
      setIsTerminalLoading(false);
    }, 800);
    return () => clearTimeout(timer);
  }, [reducedMotion]);

  // Stop the WebGL background when hero isn't visible (prevents scroll jank).
  useEffect(() => {
    if (reducedMotion) {
      setThreadsActive(false);
      return;
    }
    const hero = heroSectionRef.current;
    if (!hero) return;

    const observer = new IntersectionObserver(
      ([entry]) => setThreadsActive(entry.isIntersecting),
      { threshold: 0.08 },
    );
    observer.observe(hero);
    return () => observer.disconnect();
  }, [reducedMotion]);

  // Hero entrance + terminal timeline.
  useEffect(() => {
    if (isTerminalLoading) return;
    if (reducedMotion) {
      setStatus("connected");
      const term = terminalBodyRef.current;
      if (term) {
        term.innerHTML = `
          <div class="terminal-line"><span class="terminal-prompt">$</span> ${TERMINAL_SEQUENCE.initialCommand}</div>
          <div class="terminal-line">${TERMINAL_SEQUENCE.connectionOutput[0]}</div>
          <div class="terminal-line">${TERMINAL_SEQUENCE.connectionOutput[1]}</div>
          <div class="terminal-line"><span class="terminal-prompt">${TERMINAL_SEQUENCE.sessionStart}</span>${TERMINAL_SEQUENCE.typedCommand}</div>
          <div class="terminal-line">${TERMINAL_SEQUENCE.deployOutput.join("</div><div class='terminal-line'>")}</div>
          <div class="terminal-line" style="color: var(--terminal-green);">Session restored. Deployment successful.</div>
          <div class="terminal-line"><span class="terminal-prompt">${TERMINAL_SEQUENCE.finalPrompt}</span><span class="terminal-cursor"></span></div>
        `;
      }
      return;
    }

    const ctx = gsap.context(() => {
      // Ensure CTAs are visible even if an animation gets interrupted.
      if (ctaRef.current) {
        gsap.set(ctaRef.current.children, { opacity: 1, y: 0 });
      }

      if (navRef.current) {
        gsap.from(navRef.current, {
          opacity: 0,
          y: -10,
          duration: 0.55,
          ease: "power2.out",
          delay: 0.1,
        });
      }

      if (heroTitleRef.current) {
        gsap.from(heroTitleRef.current, {
          opacity: 0,
          y: 16,
          filter: "blur(10px)",
          duration: 0.85,
          ease: "power3.out",
          delay: 0.05,
          onComplete: () => {
            gsap.set(heroTitleRef.current, { clearProps: "filter" });
          },
        });
      }

      if (heroUnderlineRef.current) {
        gsap.fromTo(
          heroUnderlineRef.current,
          { scaleX: 0 },
          {
            scaleX: 1,
            duration: 0.9,
            ease: "power3.out",
            delay: 0.22,
          },
        );
      }

      if (terminalContainerRef.current) {
        gsap.from(terminalContainerRef.current, {
          opacity: 0,
          y: 14,
          duration: 0.65,
          ease: "power3.out",
          delay: 0.22,
        });
      }

      if (ctaRef.current) {
        gsap.from(ctaRef.current.children, {
          opacity: 0,
          y: 12,
          duration: 0.5,
          ease: "power2.out",
          stagger: 0.12,
          delay: 0.35,
        });
      }

      if (versionRef.current) {
        gsap.from(versionRef.current, {
          opacity: 0,
          duration: 0.4,
          delay: 0.55,
        });
      }

      // Terminal build DOM once, then animate.
      setStatus("connected");

      const term = terminalBodyRef.current;
      if (!term) return;
      term.innerHTML = "";

      const makeLine = (html: string, extraClass: string = "") => {
        const el = document.createElement("div");
        el.className = `terminal-line${extraClass ? ` ${extraClass}` : ""}`;
        el.innerHTML = html === "" ? "<span>&nbsp;</span>" : html;
        term.appendChild(el);
        return el;
      };

      const hide = (el: Element) => gsap.set(el, { autoAlpha: 0 });

      // Command 1
      const cmd1Line = makeLine(
        `<span class="terminal-prompt">$</span>` +
          `<span class="terminal-typed">` +
          `<span class="terminal-typed-text"></span>` +
          `<span class="terminal-cursor terminal-typing-cursor terminal-typing-cursor-1" aria-hidden="true"></span>` +
          `</span>`,
      );
      const cmd1Text = cmd1Line.querySelector<HTMLElement>(".terminal-typed-text");
      const cmd1Cursor = cmd1Line.querySelector<HTMLElement>(".terminal-typing-cursor-1");

      const connectionLines = TERMINAL_SEQUENCE.connectionOutput.map((line) => {
        const el = makeLine(line);
        hide(el);
        return el;
      });

      // Command 2
      const cmd2Line = makeLine(
        `<span class="terminal-prompt">${TERMINAL_SEQUENCE.sessionStart}</span>` +
          `<span class="terminal-typed">` +
          `<span class="terminal-typed-text"></span>` +
          `<span class="terminal-cursor terminal-typing-cursor terminal-typing-cursor-2" aria-hidden="true"></span>` +
          `</span>`,
      );
      hide(cmd2Line);
      const cmd2Text = cmd2Line.querySelector<HTMLElement>(".terminal-typed-text");
      const cmd2Cursor = cmd2Line.querySelector<HTMLElement>(".terminal-typing-cursor-2");

      const deployLines = TERMINAL_SEQUENCE.deployOutput.map((line) => {
        const el = makeLine(line);
        hide(el);
        return el;
      });

      const disconnectLine = makeLine(
        `<span style="color: var(--terminal-red); font-weight: 600;">[${TERMINAL_SEQUENCE.disconnectMessage}]</span>`,
        "disconnect-msg",
      );
      hide(disconnectLine);

      const reconnectLine1 = makeLine(
        `<span style="color: var(--terminal-amber);">${TERMINAL_SEQUENCE.reconnectMessages[0]}</span>`,
      );
      const reconnectLine2 = makeLine(
        `<span style="color: var(--terminal-green);">${TERMINAL_SEQUENCE.reconnectMessages[1]}</span>`,
      );
      hide(reconnectLine1);
      hide(reconnectLine2);

      const resumeLines = TERMINAL_SEQUENCE.resumeOutput.map((line) => {
        const el = makeLine(line);
        hide(el);
        return el;
      });

      const finalPromptLine = makeLine(
        `<span class="terminal-typed">` +
          `<span class="terminal-prompt">${TERMINAL_SEQUENCE.finalPrompt}</span>` +
          `<span class="terminal-cursor" aria-hidden="true"></span>` +
          `</span>`,
      );
      hide(finalPromptLine);

      if (cmd1Text) cmd1Text.textContent = "";
      if (cmd2Text) cmd2Text.textContent = "";

      const masterTl = gsap.timeline({ delay: 0.55 });

      // Type cmd1 (solid cursor), then blink briefly, then hide cursor.
      if (cmd1Text) masterTl.add(typeText(cmd1Text, TERMINAL_SEQUENCE.initialCommand, 0.04));
      if (cmd1Cursor) {
        masterTl.add(() => cmd1Cursor.classList.remove("terminal-typing-cursor"), "+=0.02");
        masterTl.add(() => {}, "+=0.22");
        masterTl.set(cmd1Cursor, { autoAlpha: 0 });
      }

      masterTl.to(connectionLines, { autoAlpha: 1, duration: 0.01, stagger: 0.12 }, "+=0.06");

      masterTl.to(cmd2Line, { autoAlpha: 1, duration: 0.01 }, "+=0.12");
      if (cmd2Text) masterTl.add(typeText(cmd2Text, TERMINAL_SEQUENCE.typedCommand, 0.035));
      if (cmd2Cursor) {
        masterTl.add(() => cmd2Cursor.classList.remove("terminal-typing-cursor"), "+=0.02");
        masterTl.add(() => {}, "+=0.22");
        masterTl.set(cmd2Cursor, { autoAlpha: 0 });
      }

      masterTl.to(deployLines, { autoAlpha: 1, duration: 0.01, stagger: 0.14 }, "+=0.18");

      // Disconnect
      masterTl.add(() => {
        setStatus("disconnected");
        flashRef.current?.classList.add("active");
        glitchContainerRef.current?.classList.add("glitch-active", "terminal-shake");
        scanlineRef.current?.classList.add("active");
      }, "+=1.25");
      masterTl.set(disconnectLine, { autoAlpha: 1 });
      masterTl.add(() => {
        flashRef.current?.classList.remove("active");
      }, "+=0.2");

      // Hold glitch
      masterTl.add(() => {}, "+=0.8");

      // Reconnect
      masterTl.add(() => {
        setStatus("reconnecting");
        glitchContainerRef.current?.classList.remove("glitch-active", "terminal-shake");
        scanlineRef.current?.classList.remove("active");
      }, "+=0.25");
      masterTl.set(reconnectLine1, { autoAlpha: 1 });

      masterTl.add(() => {
        setStatus("connected");
      }, "+=0.85");
      masterTl.set(reconnectLine2, { autoAlpha: 1 });

      // Resume output
      masterTl.to(resumeLines, { autoAlpha: 1, duration: 0.01, stagger: 0.11 }, "+=0.35");

      // Final prompt with blinking cursor
      masterTl.set(finalPromptLine, { autoAlpha: 1 }, "+=1.05");
    });

    return () => {
      ctx.revert();
    };
  }, [isTerminalLoading, reducedMotion, setStatus, typeText]);

  // Cascading scroll reveals for the rest of the page.
  useEffect(() => {
    if (reducedMotion) return;

    const groups = Array.from(
      document.querySelectorAll<HTMLElement>("[data-reveal-group]"),
    );

    const getItems = (group: HTMLElement) => {
      const items = Array.from(
        group.querySelectorAll<HTMLElement>("[data-reveal-item]"),
      );
      return items.length ? items : [group];
    };

    for (const group of groups) {
      const items = getItems(group);
      gsap.set(items, { autoAlpha: 0, x: 18, filter: "blur(6px)" });
    }

    const observer = new IntersectionObserver(
      (entries) => {
        for (const entry of entries) {
          if (!entry.isIntersecting) continue;
          const group = entry.target as HTMLElement;
          observer.unobserve(group);
          const items = getItems(group);

          gsap.to(items, {
            autoAlpha: 1,
            x: 0,
            filter: "blur(0px)",
            duration: 0.7,
            ease: "power3.out",
            stagger: 0.085,
            onComplete: () => {
              gsap.set(items, { clearProps: "filter" });
            },
          });
        }
      },
      { threshold: 0.18 },
    );

    for (const group of groups) observer.observe(group);
    return () => observer.disconnect();
  }, [reducedMotion]);

  return (
    <div className="min-h-screen relative">
      <div className="bg-dot-grid" />
      <div className="grain" />
      <div ref={scanlineRef} className="scanlines" />

      {/* Navigation */}
      <nav
        ref={navRef}
        className="fixed top-0 left-0 right-0 z-[10050] pointer-events-auto border-b border-white/5 bg-[rgba(13,14,16,0.86)] backdrop-blur-md"
      >
        <div className="max-w-6xl mx-auto px-6 py-4 flex items-center justify-between gap-4">
          <a href="/" aria-label="Home" className="shrink-0 flex items-center">
            <img
              src="/nav-icon.png"
              alt="Undying Terminal"
              className="h-7 w-auto"
            />
          </a>

          <div className="hidden md:flex items-center gap-6">
            <a
              href="#features"
              className="nav-link text-sm text-muted-foreground hover:text-foreground transition-colors"
            >
              Features
            </a>
            <a
              href="#faq"
              className="nav-link text-sm text-muted-foreground hover:text-foreground transition-colors"
            >
              FAQ
            </a>
            <a
              href="https://undyingterminal.com/docs/introduction"
              className="nav-link text-sm text-muted-foreground hover:text-foreground transition-colors"
            >
              Docs
            </a>
            <a
              href="https://github.com/Microck/UndyingTerminal"
              className="nav-link text-sm text-muted-foreground hover:text-foreground transition-colors"
            >
              GitHub
            </a>
          </div>

          <div className="flex items-center gap-3">
            <a
              href="https://github.com/Microck/UndyingTerminal/releases/latest"
              className="hidden sm:inline-flex items-center justify-center px-4 py-2 text-sm font-medium border border-white/10 text-foreground hover:border-[var(--brand)] hover:text-white transition-colors"
            >
              Download
            </a>
            <SmoothDropdown className="md:hidden" />
          </div>
        </div>
      </nav>

      {/* Hero */}
      <section
        ref={heroSectionRef}
        className="min-h-screen flex flex-col justify-center px-6 pt-24 pb-16 relative"
      >
        {threadsActive ? (
          <div className="absolute inset-0 z-0 overflow-hidden">
            <Threads
              color={[0.99, 0.4, 0.02]}
              amplitude={0.7}
              distance={0.28}
              enableMouseInteraction={true}
            />
            <LightRays
              className="opacity-55"
              color="rgba(252, 101, 5, 0.22)"
              blur={44}
              speed={16}
              count={6}
              length="62vh"
            />
          </div>
        ) : null}

        <div className="bg-hero-glow" />

        <div className="max-w-6xl mx-auto w-full relative z-10">
          <p className="text-sm text-muted-foreground mb-3">
            Windows-native persistent terminal sessions.
          </p>

          <h1
            ref={heroTitleRef}
            className="text-4xl sm:text-5xl md:text-6xl font-semibold tracking-tight"
          >
            {!heroDecryptDone ? (
              <DecryptedText
                text={TITLE_TEXT}
                speed={40}
                maxIterations={1}
                sequential
                animateOn="view"
                onComplete={handleHeroDecryptDone}
                className={`${GeistPixelSquare.className} hero-title-pixel-heavy`}
                encryptedClassName={`${GeistPixelSquare.className} hero-title-encrypted hero-title-pixel-heavy`}
              />
            ) : (
              <ShinyText
                text={TITLE_TEXT}
                disabled={false}
                speed={2.8}
                color="#cfcfcf"
                shineColor="#ffffff"
                spread={110}
                className={`${GeistPixelSquare.className} hero-title-pixel-shine`}
              />
            )}
          </h1>

          <div className="mt-4 mb-8 h-px bg-white/10 overflow-hidden">
            <div
              ref={heroUnderlineRef}
              className="h-px bg-[var(--brand)] origin-left scale-x-0"
            />
          </div>

          <div
            ref={terminalContainerRef}
            className="w-full"
          >
            {isTerminalLoading ? (
              <TerminalSkeleton />
            ) : (
              <>
                <div
                  ref={glitchContainerRef}
                  className="glitch-container terminal-window grid grid-cols-[minmax(100px,140px)_1fr] sm:grid-cols-[minmax(120px,160px)_1fr] w-full"
                >
                  <div ref={flashRef} className="terminal-flash" />

                  {/* Status rail */}
                  <div className="border-r border-white/5 bg-[rgba(255,255,255,0.02)] p-2 sm:p-3 font-mono text-[11px] leading-5 text-muted-foreground">
                    <div className="mb-3 text-[10px] uppercase tracking-wider text-foreground">
                      STATUS
                    </div>

                    <div className="flex items-center gap-2">
                      <span ref={statusRef} className="status-dot connected" />
                      <span ref={statusTextRef}>Connected</span>
                    </div>

                    <div className="mt-3">
                      <div ref={rttRef}>rtt: n/a</div>
                      <div ref={retriesRef}>retries: 0</div>
                      <div ref={bufferRef}>buffer: persisted</div>
                    </div>
                  </div>

                  {/* Terminal */}
                  <div className="min-w-0">
                    <div className="terminal-header">
                      <span className="text-muted-foreground">UT://</span>
                      <span className="text-foreground">prod/deploy</span>
                      <span className="text-muted-foreground">transport:</span>
                      <span className="text-foreground">ssh</span>
                      <div className="flex-1" />
                      <span className="text-muted-foreground">session:persist</span>
                    </div>

                    <div
                      ref={terminalBodyRef}
                      className="terminal-body glitch-text"
                      data-text=""
                    />
                  </div>
                </div>

                <div ref={ctaRef} className="flex flex-wrap gap-4 mt-8">
                  <a
                    href="https://github.com/Microck/UndyingTerminal/releases/latest"
                    className="btn-primary"
                  >
                    <img
                      src="/windows-logo.png"
                      alt=""
                      aria-hidden="true"
                      className="h-5 w-5"
                    />
                    Download for Windows
                  </a>
                  <a href="https://undyingterminal.com/docs/quickstart" className="btn-secondary">
                    Quick Start
                  </a>
                </div>

                <p ref={versionRef} className="text-sm text-muted-foreground mt-5">
                  Windows 10 build 17763+ &middot; MIT License
                </p>
              </>
            )}
          </div>
        </div>
      </section>

      <Features />

      <FAQs />

      {/* Final CTA */}
      <section
        className="py-24 px-6 border-t border-white/5"
        data-reveal-group
      >
        <div className="max-w-6xl mx-auto text-center">
          <h2 className="text-3xl sm:text-4xl font-semibold mb-4" data-reveal-item>
            Stop losing sessions.
          </h2>
          <p className="text-lg text-muted-foreground mb-10 max-w-md mx-auto" data-reveal-item>
            Free and open source. Download now or read the docs.
          </p>
          <div className="flex flex-wrap justify-center gap-4" data-reveal-item>
            <a href="https://github.com/Microck/UndyingTerminal/releases/latest" className="btn-primary">
              Download
            </a>
            <a href="https://undyingterminal.com/docs/introduction" className="btn-secondary">
              Documentation
            </a>
          </div>
        </div>
      </section>

      <FooterSection />
    </div>
  );
}
