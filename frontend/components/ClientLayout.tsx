"use client";

import React, { useState } from "react";
import Header from "@/components/Header";
 import Sidebar from "./ui/Sidebar"; 

export default function ClientLayout({ children }: { children: React.ReactNode }) {
  // This state controls the sidebar across the whole layout
  const [isSidebarOpen, setIsSidebarOpen] = useState(true);

  const toggleSidebar = () => setIsSidebarOpen(!isSidebarOpen);
  const closeSidebar = () => setIsSidebarOpen(false);

  return (
    <div className="flex flex-col min-h-screen w-full bg-background text-foreground font-sans">
      {/* The Header receives the toggle function */}
      <Header toggleSidebar={toggleSidebar} />
      
      <div className="flex flex-1 overflow-hidden relative">
        <Sidebar isOpen={isSidebarOpen} closeSidebar={closeSidebar} />
        
        <main 
          className={`flex-1 overflow-y-auto transition-all duration-300 ease-in-out ${
            isSidebarOpen ? 'lg:ml-64' : 'ml-0'
          }`}
        >
          {children}
        </main>
      </div>
    </div>
  );
}