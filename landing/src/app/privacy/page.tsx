import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "Privacy Policy - Undying Terminal",
  description: "Privacy policy for Undying Terminal",
};

export default function PrivacyPage() {
  return (
    <main className="min-h-screen px-6 py-24 max-w-3xl mx-auto">
      <h1 className="text-3xl font-semibold mb-8">Privacy Policy</h1>
      
      <div className="space-y-6 text-muted-foreground">
        <section>
          <h2 className="text-xl font-semibold text-foreground mb-3">Overview</h2>
          <p>
            Undying Terminal is an open-source terminal application. This privacy policy 
            explains how we handle data when you use our website and software.
          </p>
        </section>

        <section>
          <h2 className="text-xl font-semibold text-foreground mb-3">Website Usage</h2>
          <p>
            Our website (undyingterminal.com) does not use cookies or tracking scripts. 
            We do not collect personal information from visitors. The site is statically 
            hosted on Vercel, which may collect basic access logs for operational purposes.
          </p>
        </section>

        <section>
          <h2 className="text-xl font-semibold text-foreground mb-3">Software Usage</h2>
          <p>
            The Undying Terminal application itself:
          </p>
          <ul className="list-disc pl-5 mt-2 space-y-1">
            <li>Does not collect telemetry or analytics</li>
            <li>Does not transmit data to third parties</li>
            <li>Connects only to servers you specify via SSH</li>
            <li>Stores session data locally on your machine</li>
          </ul>
        </section>

        <section>
          <h2 className="text-xl font-semibold text-foreground mb-3">Third-Party Services</h2>
          <p>
            Our documentation is hosted on Mintlify. When you access docs links, 
            you are subject to Mintlify&apos;s privacy policy. Our GitHub releases 
            are hosted on GitHub and subject to their terms.
          </p>
        </section>

        <section>
          <h2 className="text-xl font-semibold text-foreground mb-3">Open Source</h2>
          <p>
            Undying Terminal is open source software licensed under the MIT License. 
            You can inspect the source code on GitHub to verify our data handling practices.
          </p>
        </section>

        <section>
          <h2 className="text-xl font-semibold text-foreground mb-3">Contact</h2>
          <p>
            For privacy questions, please open an issue on our{" "}
            <a href="https://github.com/Microck/UndyingTerminal" className="text-foreground hover:underline">
              GitHub repository
            </a>.
          </p>
        </section>

        <p className="text-sm pt-6 border-t border-white/10">
          Last updated: February 10, 2026
        </p>
      </div>
    </main>
  );
}
