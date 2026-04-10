"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import { X, Home } from "lucide-react";
import { appGroups } from "@/lib/appGroups";

interface SidebarProps {
  isOpen: boolean;
  closeSidebar: () => void;
}

export default function Sidebar({ isOpen, closeSidebar }: SidebarProps) {
  const pathname = usePathname();

  return (
    <>
      {/* Mobile Overlay */}
      <div 
        className={`fixed inset-0 bg-background/80 backdrop-blur-sm z-40 lg:hidden transition-opacity duration-300 ${
          isOpen ? 'opacity-100 pointer-events-auto' : 'opacity-0 pointer-events-none'
        }`}
        onClick={closeSidebar}
        aria-hidden="true"
      />

      {/* Sidebar Container */}
      <aside 
        className={`fixed top-0 lg:top-16 left-0 h-screen lg:h-[calc(100vh-4rem)] w-64 bg-card border-r border-border z-50 transform transition-transform duration-300 ease-in-out flex flex-col shadow-lg lg:shadow-none ${
          isOpen ? 'translate-x-0' : '-translate-x-full'
        }`}
      >
        {/* Mobile Header (Hidden on Desktop) */}
        <div className="flex items-center justify-between p-4 lg:hidden border-b border-border bg-surface">
          <span className="text-xl font-bold font-mono text-foreground tracking-tight">
            <span className="text-primary">CV</span>Lab
          </span>
          <button 
            onClick={closeSidebar} 
            className="text-muted-foreground hover:text-foreground transition-colors p-1"
            aria-label="Close Sidebar"
          >
            <X size={24} />
          </button>
        </div>

        {/* Navigation Content */}
        <div className="flex-1 overflow-y-auto py-6 custom-scrollbar w-full">
          {/* Static Home Link */}
          <div className="px-3 mb-6">
            <Link 
              href="/"
              onClick={() => {
                if (window.innerWidth < 1024) closeSidebar();
              }}
              className={`w-full flex items-center gap-3 px-3 py-2.5 rounded-md font-mono text-sm transition-colors ${
                pathname === "/" 
                  ? "bg-primary/10 text-primary border border-primary/20" 
                  : "text-muted-foreground hover:text-foreground hover:bg-surface"
              }`}
            >
              <Home size={18} />
              <span>Dashboard</span>
            </Link>
          </div>

          {/* Categorized Navigation Links */}
          {appGroups.map((group, idx) => (
            <div key={idx} className="mb-6 px-3">
              <h3 className="px-4 mb-2 text-xs font-semibold text-muted-foreground uppercase tracking-wider font-mono">
                {group.category}
              </h3>
              <ul className="space-y-1">
                {group.tools.map(tool => {
                  const isActive = pathname.startsWith(tool.href);
                  
                  return (
                    <li key={tool.href}>
                      <Link 
                        href={tool.href}
                        onClick={() => {
                          // Automatically close sidebar on mobile when a link is clicked
                          if (window.innerWidth < 1024) closeSidebar();
                        }}
                        className={`w-full flex items-center gap-3 px-4 py-2 rounded-md font-mono text-sm transition-all group ${
                          isActive 
                            ? "bg-primary/10 text-primary border border-primary/20" 
                            : "text-muted-foreground hover:text-foreground hover:bg-surface border border-transparent"
                        }`}
                      >
                        <span className={`${isActive ? "text-primary" : "text-muted-foreground group-hover:text-primary"} transition-colors`}>
                          {tool.icon}
                        </span>
                        <span className="flex-1 truncate">{tool.title}</span>
                      </Link>
                    </li>
                  );
                })}
              </ul>
            </div>
          ))}
        </div>
      </aside>
    </>
  );
}