import type { Metadata } from "next";
import { Inter, JetBrains_Mono } from "next/font/google";
import { Suspense } from "react";
import Header from "@/components/Header";
import { ImageProvider } from "@/contexts/ImageContext";
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
  title: "CVLab",
  description: "Computer Vision Laboratory — Image Processing Toolkit",
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en">
      <body
        className={`${inter.variable} ${jetbrainsMono.variable} antialiased min-h-screen flex flex-col`}
      >
        <Header />
        <ImageProvider>
          <Suspense
            fallback={
              <main className="flex-1 flex items-center justify-center p-6">
                <div className="h-10 w-10 rounded-full border-2 border-primary border-t-transparent animate-spin" />
              </main>
            }
          >
            {children}
          </Suspense>
        </ImageProvider>
      </body>
    </html>
  );
}
