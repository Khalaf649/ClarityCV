"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";

const navItems = [
  { href: "/", label: "Home" },
  { href: "/spatial", label: "Spatial Filters" },
  { href: "/histogram", label: "Histogram" },
  { href: "/frequency-domain", label: "Frequency Domain" },
  { href: "/hough_transform", label: "Hough Transform" },
];

export default function Header() {
  const pathname = usePathname();

  return (
    <header className="border-b border-border px-6 py-4 flex items-center justify-between">
      <h1 className="text-lg font-mono font-bold text-primary tracking-tight">
        CV<span className="text-foreground">Lab</span>
      </h1>

      <nav className="flex gap-1">
        {navItems.map((item) => {
          const isActive =
            item.href === "/"
              ? pathname === "/"
              : pathname.startsWith(item.href);

          return (
            <Link
              key={item.href}
              href={item.href}
              className={`px-4 py-2 text-sm font-mono rounded-md transition-colors ${
                isActive
                  ? "bg-primary/15 text-primary"
                  : "text-muted-foreground hover:text-foreground hover:bg-secondary"
              }`}
            >
              {item.label}
            </Link>
          );
        })}
      </nav>
    </header>
  );
}
