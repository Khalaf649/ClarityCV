"use client";

import { ControlPanel } from "@/components/ControlPanel";

interface StatisticsPanelProps {
  perimeter: number | null;
  area: number | null;
}

export function StatisticsPanel({ perimeter, area }: StatisticsPanelProps) {
  return (
    <ControlPanel title="Results">
      <div className="space-y-4">
        <div className="border-b pb-3">
          <label className="text-xs text-muted-foreground">Perimeter</label>
          <p className="text-lg font-semibold mt-1">
            {perimeter !== null ? perimeter.toFixed(2) : "-"} pixels
          </p>
        </div>

        <div>
          <label className="text-xs text-muted-foreground">Area</label>
          <p className="text-lg font-semibold mt-1">
            {area !== null ? area.toFixed(2) : "-"} pixels²
          </p>
        </div>
      </div>
    </ControlPanel>
  );
}
