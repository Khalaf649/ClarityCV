"use client";

import { useState } from "react";
import { FrequencyFilterTab } from "@/components/FrequencyFilterTab";
import { HybridImageTab } from "@/components/HybridImageTab";

type SubTab = "filter" | "hybrid";

export default function FrequencyDomainPage() {
  const [subTab, setSubTab] = useState<SubTab>("filter");

  return (
    <main className="flex-1 p-6 overflow-auto">
      <div className="flex flex-col gap-4">
        {/* Sub-tab navigation */}
        <div className="flex gap-1 border-b border-border pb-2">
          <button
            onClick={() => setSubTab("filter")}
            className={`px-3 py-1.5 text-xs font-mono rounded-t-md transition-colors cursor-pointer ${
              subTab === "filter"
                ? "bg-primary/15 text-primary border-b-2 border-primary"
                : "text-muted-foreground hover:text-foreground"
            }`}
          >
            Low/High Pass
          </button>
          <button
            onClick={() => setSubTab("hybrid")}
            className={`px-3 py-1.5 text-xs font-mono rounded-t-md transition-colors cursor-pointer ${
              subTab === "hybrid"
                ? "bg-primary/15 text-primary border-b-2 border-primary"
                : "text-muted-foreground hover:text-foreground"
            }`}
          >
            Hybrid Image
          </button>
        </div>

        {subTab === "filter" && <FrequencyFilterTab />}
        {subTab === "hybrid" && <HybridImageTab />}
      </div>
    </main>
  );
}
