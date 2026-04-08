"use client";

import { ControlPanel } from "@/components/ControlPanel";

interface StatisticsPanelProps {
  computationTime: number | null;
  keypointsCount: number | null;
}

export function StatisticsPanel({ computationTime, keypointsCount }: StatisticsPanelProps) {
  return (
    <ControlPanel title="Statistics" className="space-y-2">
      <div className="flex justify-between items-center py-1">
        <span className="text-xs text-muted-foreground">Computation Time</span>
        <span className="text-sm font-mono text-primary">{computationTime !== null ? `${computationTime} ms` : "-"}</span>
      </div>

      <div className="flex justify-between items-center py-1 border-t border-secondary">
        <span className="text-xs text-muted-foreground">Keypoints</span>
        <span className="text-sm font-mono text-primary">{keypointsCount !== null ? keypointsCount : "-"}</span>
      </div>
    </ControlPanel>
  );
}
