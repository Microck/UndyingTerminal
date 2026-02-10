import { Card, CardContent, CardHeader } from "@/components/ui/card";
import { Lock, RotateCcw, Terminal } from "lucide-react";
import type { ReactNode } from "react";

export default function Features() {
  return (
    <section
      id="features"
      className="py-16 md:py-32 border-t border-white/5 scroll-mt-28"
      data-reveal-group
    >
      <div className="@container mx-auto max-w-5xl px-6">
        <div className="text-center" data-reveal-item>
          <h2 className="text-balance text-4xl font-semibold lg:text-5xl">
            Built to survive disconnects
          </h2>
          <p className="mt-4 text-muted-foreground">
            Sessions persist on the server and reconnect over SSH, so you keep
            working through sleep, VPN flips, and network drops.
          </p>
        </div>

        <div className="@min-4xl:max-w-full @min-4xl:grid-cols-3 mx-auto mt-8 grid max-w-sm gap-6 *:text-center md:mt-16">
          <Card className="group shadow-zinc-950/5" data-reveal-item>
            <CardHeader className="pb-3">
              <CardDecorator>
                <RotateCcw className="size-6 text-primary" aria-hidden />
              </CardDecorator>
              <h3 className="mt-6 font-semibold tracking-tight">
                Persistent
              </h3>
            </CardHeader>
            <CardContent>
              <p className="text-sm text-muted-foreground">
                Your shell keeps running on the server. Disconnect and come back
                later: same state, same prompt.
              </p>
            </CardContent>
          </Card>

          <Card className="group shadow-zinc-950/5" data-reveal-item>
            <CardHeader className="pb-3">
              <CardDecorator>
                <Lock className="size-6 text-primary" aria-hidden />
              </CardDecorator>
              <h3 className="mt-6 font-semibold tracking-tight">
                Encrypted
              </h3>
            </CardHeader>
            <CardContent>
              <p className="mt-3 text-sm text-muted-foreground">
                Transport is SSH: authenticated and encrypted end-to-end. No
                cloud relay, no third parties.
              </p>
            </CardContent>
          </Card>

          <Card className="group shadow-zinc-950/5" data-reveal-item>
            <CardHeader className="pb-3">
              <CardDecorator>
                <Terminal className="size-6 text-primary" aria-hidden />
              </CardDecorator>
              <h3 className="mt-6 font-semibold tracking-tight">
                Windows-native
              </h3>
            </CardHeader>
            <CardContent>
              <p className="mt-3 text-sm text-muted-foreground">
                Built for Windows with ConPTY support. No WSL required. A single
                binary that runs anywhere.
              </p>
            </CardContent>
          </Card>
        </div>
      </div>
    </section>
  );
}

const CardDecorator = ({ children }: { children: ReactNode }) => (
  <div className="relative mx-auto size-28 border border-border bg-muted/30">
    <div
      aria-hidden
      className="absolute inset-0 bg-[linear-gradient(to_right,rgba(255,255,255,0.06)_1px,transparent_1px),linear-gradient(to_bottom,rgba(255,255,255,0.06)_1px,transparent_1px)] bg-[size:22px_22px] opacity-60"
    />

    <div className="bg-background absolute inset-0 m-auto flex size-12 items-center justify-center border border-border">
      {children}
    </div>
  </div>
);
