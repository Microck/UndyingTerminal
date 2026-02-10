import { Logo } from "@/components/logo";
import Link from "next/link";

const links = [
  { title: "Features", href: "#features" },
  { title: "Quickstart", href: "https://undyingterminal.com/docs/quickstart", external: true },
  { title: "Docs", href: "https://undyingterminal.com/docs/introduction", external: true },
  { title: "Releases", href: "https://github.com/Microck/UndyingTerminal/releases/latest", external: true },
  { title: "GitHub", href: "https://github.com/Microck/UndyingTerminal", external: true },
  { title: "FAQ", href: "#faq" },
  { title: "Privacy", href: "/privacy" },
];

export default function FooterSection() {
  return (
    <footer className="py-16 md:py-24 border-t border-white/5">
      <div className="mx-auto max-w-5xl px-6">
        <Link href="/" aria-label="go home" className="mx-auto block size-fit">
          <Logo />
        </Link>

        <div className="my-10 flex flex-wrap justify-center gap-6 text-sm">
          {links.map((link) => {
            const common = "text-muted-foreground hover:text-primary block duration-150";
            if (link.external) {
              return (
                <a
                  key={link.title}
                  href={link.href}
                  target="_blank"
                  rel="noopener noreferrer"
                  className={common}
                >
                  <span>{link.title}</span>
                </a>
              );
            }
            return (
              <Link key={link.title} href={link.href} className={common}>
                <span>{link.title}</span>
              </Link>
            );
          })}
        </div>

        <span className="text-muted-foreground block text-center text-sm">
          © {new Date().getFullYear()} Undying Terminal — MIT License
        </span>
      </div>
    </footer>
  );
}
