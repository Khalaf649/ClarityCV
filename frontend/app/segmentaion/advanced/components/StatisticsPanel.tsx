"use client";

import { ControlPanel } from "@/components/ControlPanel";

interface StatisticsPanelProps {
  computationTime: number | null;
  metadata: Record<string, unknown> | null;
  method: string | null;
}

function MetaRow({ label, value }: { label: string; value: unknown }) {
  if (value === undefined || value === null) return null;
  let display: string;
  if (Array.isArray(value)) {
    display = value.join(", ");
  } else if (typeof value === "object") {
    display = JSON.stringify(value);
  } else if (typeof value === "number") {
    display = Number.isInteger(value) ? String(value) : value.toFixed(3);
  } else {
    display = String(value);
  }
  return (
    <div>
      <label className="text-xs text-muted-foreground">{label}</label>
      <p className="text-base font-mono font-semibold text-cyan-400 mt-1">
        {display}
      </p>
    </div>
  );
}

export function StatisticsPanel({
  computationTime,
  metadata,
  method,
}: StatisticsPanelProps) {
  return (
    <ControlPanel title="Statistics">
      <div className="space-y-4">
        <div>
          <label className="text-xs text-muted-foreground">
            Computation Time
          </label>
          <p className="text-lg font-mono font-semibold text-foreground mt-1">
            {computationTime !== null ? `${computationTime.toFixed(3)}s` : "N/A"}
          </p>
        </div>

        {method && (
          <div>
            <label className="text-xs text-muted-foreground">Method</label>
            <p className="text-sm font-mono mt-1">{method}</p>
          </div>
        )}

        {metadata && (
          <>
            <MetaRow label="Clusters" value={metadata.clusters} />
            <MetaRow label="Labels found" value={metadata.labels_count} />
            <MetaRow label="Threshold" value={metadata.threshold} />
            <MetaRow label="Bandwidth" value={metadata.bandwidth} />
            <MetaRow label="Seed point" value={metadata.seed_point} />
          </>
        )}
      </div>
    </ControlPanel>
  );
}
