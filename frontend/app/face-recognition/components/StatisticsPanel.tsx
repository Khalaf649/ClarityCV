"use client";

import { ControlPanel } from "@/components/ControlPanel";

interface StatisticsPanelProps {
  computationTime: number | null;
  facesDetected: number | null;
  method: string | null;
}

export function StatisticsPanel({
  computationTime,
  facesDetected,
  method,
}: StatisticsPanelProps) {
  return (
    <ControlPanel title="Statistics">
      <div className="space-y-4">
        {/* Computation Time */}
        <div>
          <label className="text-xs text-muted-foreground">
            Computation Time
          </label>
          <p className="text-lg font-mono font-semibold text-foreground mt-1">
            {computationTime !== null
              ? `${computationTime.toFixed(3)}s`
              : "N/A"}
          </p>
        </div>

        {/* Face Count - Rendered based on method */}
        {method === "face_recognition" && (
          <div>
            <label className="text-xs text-muted-foreground">
              Faces Recognized
            </label>
            <p className="text-lg font-mono font-semibold text-cyan-400 mt-1">
              {facesDetected !== null ? facesDetected : "N/A"}
            </p>
          </div>
        )}

        {method === "haar_detection" && (
          <div>
            <label className="text-xs text-muted-foreground">
              Faces Detected
            </label>
            <p className="text-lg font-mono font-semibold text-cyan-400 mt-1">
              {facesDetected !== null ? facesDetected : "N/A"}
            </p>
          </div>
        )}
      </div>
    </ControlPanel>
  );
}
