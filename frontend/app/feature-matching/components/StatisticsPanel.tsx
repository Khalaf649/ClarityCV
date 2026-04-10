"use client";

import { ControlPanel } from "@/components/ControlPanel";

interface StatisticsPanelProps {
  matchesCount:   number | null;
  selectedMethod: string;
  computationTime?: number | null;
  siftTimeImg1?:   number | null;
  siftTimeImg2?:   number | null;
  keypointsImg1?:  number | null;
  keypointsImg2?:  number | null;
}

function Row({ label, value }: { label: string; value: string }) {
  return (
    <div className="flex justify-between items-center py-1 border-t border-secondary">
      <span className="text-xs text-muted-foreground">{label}</span>
      <span className="text-sm font-mono text-primary">{value}</span>
    </div>
  );
}

function fmt(n: number | null | undefined, unit = "") {
  return n != null ? `${n}${unit}` : "–";
}

export function StatisticsPanel({
  matchesCount,
  selectedMethod,
  computationTime,
  siftTimeImg1,
  siftTimeImg2,
  keypointsImg1,
  keypointsImg2,
}: StatisticsPanelProps) {
  return (
    <ControlPanel title="Statistics" className="space-y-2">
      <div className="flex justify-between items-center py-1">
        <span className="text-xs text-muted-foreground">Method</span>
        <span className="text-sm font-mono text-primary">{selectedMethod}</span>
      </div>

      <Row label="Good Matches"      value={fmt(matchesCount)} />
      <Row label="Keypoints (Img 1)" value={fmt(keypointsImg1)} />
      <Row label="Keypoints (Img 2)" value={fmt(keypointsImg2)} />
      <Row label="SIFT Time (Img 1)" value={fmt(siftTimeImg1, " ms")} />
      <Row label="SIFT Time (Img 2)" value={fmt(siftTimeImg2, " ms")} />
      <Row label="Matching Time"     value={fmt(computationTime, " ms")} />
    </ControlPanel>
  );
}
