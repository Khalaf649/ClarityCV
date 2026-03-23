"use client";

import { ControlPanel } from "@/components/ControlPanel";

interface StatisticsPanelProps {
  perimeter: number | null;
  area: number | null;
  chainCode?: string | null;
}

export function StatisticsPanel({ perimeter, area, chainCode }: StatisticsPanelProps) {
  const formatChainCode = (code: string): string => {
    // Insert spaces every 50 characters for readability
    if (!code || code.length === 0) return "";
    const parts = [];
    for (let i = 0; i < code.length; i += 50) {
      parts.push(code.substring(i, i + 50));
    }
    return parts.join("\n");
  };

  return (
    <ControlPanel title="Results">
      <div className="space-y-4">
        <div className="border-b pb-3">
          <label className="text-xs text-muted-foreground">Perimeter</label>
          <p className="text-lg font-semibold mt-1">
            {perimeter !== null ? perimeter.toFixed(2) : "-"} pixels
          </p>
        </div>

        <div className="border-b pb-3">
          <label className="text-xs text-muted-foreground">Area</label>
          <p className="text-lg font-semibold mt-1">
            {area !== null ? area.toFixed(2) : "-"} pixels²
          </p>
        </div>

        {chainCode && (
          <div className="pt-2">
            <label className="text-xs text-muted-foreground">Chain Code (8-Connectivity)</label>
            <div className="mt-2 p-2 bg-muted rounded border border-border text-xs overflow-auto max-h-24 font-mono whitespace-pre-wrap break-words">
              {formatChainCode(chainCode)}
            </div>
            <p className="text-xs text-muted-foreground mt-2">
              Length: {chainCode.length}
            </p>
            <p className="text-xs text-muted-foreground mt-1">
              <span className="font-semibold">Directions:</span> 0=E 1=NE 2=N 3=NW 4=W 5=SW 6=S 7=SE
            </p>
          </div>
        )}
      </div>
    </ControlPanel>
  );
}
