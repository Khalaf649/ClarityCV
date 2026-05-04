import type { Metadata } from "next";
import { Inter, JetBrains_Mono } from "next/font/google";
import { Suspense } from "react";
import { ImageProvider } from "@/contexts/ImageContext";
import ClientLayout from "../components/ClientLayout";
import "./globals.css";

const inter = Inter({
  subsets: ["latin"],
  variable: "--font-inter",
});

const jetbrainsMono = JetBrains_Mono({
  subsets: ["latin"],
  variable: "--font-jetbrains-mono",
});

export const metadata: Metadata = {
  title: "ClairtyCv",
  description: "ClairtyCv — Computer Vision Laboratory Toolkit",
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en">
      <body
        // Added the dark theme background, text colors, and selection color here!
        className={`${inter.variable} ${jetbrainsMono.variable} antialiased min-h-screen flex flex-col bg-[#0f172a] text-slate-300 selection:bg-cyan-500/30`}
      >
        <ImageProvider>
          {/* Wrapping the app in the Client Layout to handle Sidebar state */}
          <ClientLayout>
            <Suspense
              fallback={
                <div className="flex-1 flex items-center justify-center p-6">
                  {/* Updated the loading spinner to match the cyan theme */}
                  <div className="h-10 w-10 rounded-full border-2 border-cyan-400 border-t-transparent animate-spin" />
                </div>
              }
            >
              {children}
            </Suspense>
          </ClientLayout>
        </ImageProvider>
      </body>
    </html>
  );
}
