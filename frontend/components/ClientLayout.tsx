"use client";

import React, { useState } from "react";
import Header from "@/components/Header";
import Sidebar from "./ui/Sidebar";

export default function ClientLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  // This state controls the sidebar across the whole layout
  const [isSidebarOpen, setIsSidebarOpen] = useState(true);

  const toggleSidebar = () => setIsSidebarOpen(!isSidebarOpen);
  const closeSidebar = () => setIsSidebarOpen(false);

  return (
    <div className="flex flex-col min-h-screen w-full bg-background text-foreground font-sans">
      {/* Header with toggle function */}
      <Header toggleSidebar={toggleSidebar} isSidebarOpen={isSidebarOpen} />

      {/* Flex container for Sidebar and Main - flex-row layout */}
      <div className="flex flex-row flex-1 overflow-hidden relative">
        {/* Sidebar - fixed width when open */}
        <div
          className={`transition-all duration-300 ease-in-out ${
            isSidebarOpen ? "w-64" : "w-0"
          }`}
        >
          <Sidebar isOpen={isSidebarOpen} closeSidebar={closeSidebar} />
        </div>

        {/* Main content - responsive width */}
        <main
          className={`overflow-y-auto transition-all duration-300 ease-in-out ${
            isSidebarOpen ? "w-[calc(100vw-16rem)]" : "w-screen"
          }`}
        >
          {children}
        </main>
      </div>
    </div>
  );
}
