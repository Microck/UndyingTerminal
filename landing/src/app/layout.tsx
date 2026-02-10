import type { Metadata, Viewport } from "next";
import "./globals.css";

export const viewport: Viewport = {
  themeColor: "#0d0e10",
};

export const metadata: Metadata = {
  metadataBase: new URL("https://undyingterminal.com"),
  title: "Undying Terminal",
  description: "Your session survives. Windows-native persistent terminal that reconnects through disconnects, sleep, and network changes.",
  keywords: ["terminal", "ssh", "persistent", "windows", "reconnect", "shell"],
  authors: [{ name: "Microck" }],
  alternates: {
    canonical: "/",
  },
  openGraph: {
    title: "Undying Terminal",
    description: "Your session survives.",
    type: "website",
    url: "https://undyingterminal.com",
    images: [
      {
        url: "/icon-512.png",
        width: 512,
        height: 512,
        alt: "Undying Terminal",
      },
    ],
  },
  twitter: {
    card: "summary_large_image",
    title: "Undying Terminal",
    description: "Your session survives.",
    images: ["/icon-512.png"],
  },
  manifest: "/manifest.json",
  icons: {
    icon: "/favicon.ico",
    apple: "/apple-touch-icon.png",
  },
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en" className="dark">
      <head>
        <link rel="preconnect" href="https://fonts.googleapis.com" />
        <link rel="preconnect" href="https://fonts.gstatic.com" crossOrigin="anonymous" />
        <script defer data-domain="undyingterminal.com" src="https://plausible.io/js/script.js"></script>
      </head>
      <body className="isolate">
        {children}
      </body>
    </html>
  );
}
