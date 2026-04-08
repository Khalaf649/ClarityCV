"use client";

import { ControlPanel } from "@/components/ControlPanel";

interface StatisticsPanelProps {
  matchesCount: number | null;
  selectedMethod: string;
}

export function StatisticsPanel({ matchesCount, selectedMethod }: StatisticsPanelProps) {
  return (
    <ControlPanel title="Statistics" className="space-y-2">
      <div className="flex justify-between items-center py-1">
        <span className="text-xs text-muted-foreground">Method</span>
        <span className="text-sm font-mono text-primary">{selectedMethod}</span>
      </div>

      <div className="flex justify-between items-center py-1 border-t border-secondary">
        <span className="text-xs text-muted-foreground">Matches</span>
        <span className="text-sm font-mono text-primary">{matchesCount !== null ? matchesCount : "-"}</span>
      </div>
    </ControlPanel>
  );
}
