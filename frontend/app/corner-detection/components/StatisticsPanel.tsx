"use client";

import { ControlPanel } from "@/components/ControlPanel";

interface StatisticsPanelProps {
  computationTime: number | null;
  featureCount: number | null;
}

export function StatisticsPanel({
  computationTime,
  featureCount,
}: StatisticsPanelProps) {
  return (
    <ControlPanel title="Statistics" className="space-y-2">
      <div className="flex justify-between items-center py-1">
        <span className="text-xs text-muted-foreground">Computation Time</span>
        <span className="text-sm font-mono text-primary">
          {computationTime !== null ? `${computationTime} ms` : "-"}
        </span>
      </div>
      <div className="flex justify-between items-center py-1 border-t border-secondary">
        <span className="text-xs text-muted-foreground">Feature Count</span>
        <span className="text-sm font-mono text-primary">
          {featureCount !== null ? featureCount : "-"}
        </span>
      </div>
    </ControlPanel>
  );
}
