"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import { Suspense, type ReactNode } from "react";

const subNavItems = [
  { href: "/frequency-domain/low-high-pass", label: "Low / High Pass" },
  { href: "/frequency-domain/hybrid-image", label: "Hybrid Image" },
];

export default function FrequencyDomainLayout({
  children,
}: {
  children: ReactNode;
}) {
  const pathname = usePathname();

  return (
    <div className="flex-1 flex flex-col p-6 overflow-auto gap-4">
      {/* Sub-navigation */}
      <nav className="flex gap-1 border-b border-border pb-2">
        {subNavItems.map((item) => {
          const isActive = pathname.startsWith(item.href);
          return (
            <Link
              key={item.href}
              href={item.href}
              className={`px-3 py-1.5 text-xs font-mono rounded-t-md transition-colors ${
                isActive
                  ? "bg-primary/15 text-primary border-b-2 border-primary"
                  : "text-muted-foreground hover:text-foreground"
              }`}
            >
              {item.label}
            </Link>
          );
        })}
      </nav>

      <Suspense
        fallback={
          <div className="flex-1 flex items-center justify-center">
            <div className="h-8 w-8 rounded-full border-2 border-primary border-t-transparent animate-spin" />
          </div>
        }
      >
        {children}
      </Suspense>
    </div>
  );
}
