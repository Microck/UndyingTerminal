export default function FAQs() {
  return (
    <section
      id="faq"
      className="scroll-py-16 py-16 md:scroll-py-32 md:py-32 border-t border-white/5 scroll-mt-28"
      data-reveal-group
    >
      <div className="mx-auto max-w-5xl px-6">
        <div className="grid gap-y-12 px-2 lg:[grid-template-columns:1fr_auto]">
          <div className="text-center lg:text-left" data-reveal-item>
            <h2 className="mb-4 text-3xl font-semibold md:text-4xl">
              Questions, answered.
            </h2>
            <p className="text-muted-foreground">
              Undying Terminal is small on purpose. Here are the sharp edges up
              front.
            </p>
          </div>

          <div className="divide-y divide-dashed sm:mx-auto sm:max-w-lg lg:mx-0">
            <div className="pb-6" data-reveal-item>
              <h3 className="font-semibold">Is this a tmux replacement?</h3>
              <p className="text-muted-foreground mt-4">
                Similar goal (don&apos;t lose state), different focus: Windows-native
                client + server-persisted sessions. You can still use tmux/screen
                inside the session if you want.
              </p>
            </div>

            <div className="py-6" data-reveal-item>
              <h3 className="font-semibold">Is the connection encrypted?</h3>
              <p className="text-muted-foreground mt-4">
                Yes. Traffic runs over SSH so it&apos;s authenticated and encrypted.
                You bring your own server; nothing is routed through a third
                party.
              </p>
            </div>

            <div className="py-6" data-reveal-item>
              <h3 className="font-semibold">Do I need WSL?</h3>
              <p className="text-muted-foreground mt-4">
                No. Undying Terminal is built for Windows and uses ConPTY, so it
                behaves like a native terminal.
              </p>
            </div>

            <div className="py-6" data-reveal-item>
              <h3 className="font-semibold">What happens during a network drop?</h3>
              <p className="text-muted-foreground mt-4">
                The session stays alive on the server. When the client comes
                back, it reconnects and resumes where you left off.
              </p>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
