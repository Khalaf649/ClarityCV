"use client";

import Link from "next/link";
import { Menu, Github } from "lucide-react";

interface HeaderProps {
  toggleSidebar: () => void;
}

export default function Header({ toggleSidebar }: HeaderProps) {
  return (
    <header className="h-16 bg-slate-900 border-b border-slate-800 flex items-center justify-between px-4 sticky top-0 z-40">
      <div className="flex items-center gap-4">
        {/* Hamburger Menu Button */}
        <button
          onClick={toggleSidebar}
          className="p-2 text-slate-400 hover:text-cyan-400 hover:bg-slate-800 rounded-lg transition-colors focus:outline-none focus:ring-2 focus:ring-cyan-500/50"
          aria-label="Toggle Sidebar"
        >
          <Menu size={24} />
        </button>
        
        {/* Logo */}
        <Link 
          href="/" 
          className="text-2xl font-bold text-white flex items-center no-underline hover:opacity-80 transition-opacity"
        >
          <span className="text-cyan-400 font-mono tracking-tight">Clairty</span>
          <span className="font-mono tracking-tight text-white">Cv</span>
        </Link>
      </div>

      {/* Global Actions */}
      <div className="flex items-center gap-4">
        <a 
          href="https://github.com/Khalaf649/ClarityCV" 
          target="_blank" 
          rel="noopener noreferrer"
          className="p-2 text-slate-400 hover:text-white hover:bg-slate-800 rounded-lg transition-colors"
          aria-label="GitHub Repository"
        >
          <Github size={20} />
        </a>
      </div>
    </header>
  );
}