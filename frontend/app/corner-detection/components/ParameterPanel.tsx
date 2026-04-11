"use client";

import { ControlPanel } from "@/components/ControlPanel";
import { Slider } from "@/components/ui/Slider";
import { Select } from "@/components/ui/Select";

type CornerDetectionMode = "Harris" | "Shi-Tomasi";

interface ParameterPanelProps {
  mode: CornerDetectionMode;
  sigma: number;
  windowSize: number;
  threshold: number;
  k: number;
  onModeChange: (mode: CornerDetectionMode) => void;
  onSigmaChange: (val: number) => void;
  onWindowSizeChange: (val: number) => void;
  onThresholdChange: (val: number) => void;
  onKChange: (val: number) => void;
}

export function ParameterPanel({
  mode,
  sigma,
  windowSize,
  threshold,
  k,
  onModeChange,
  onSigmaChange,
  onWindowSizeChange,
  onThresholdChange,
  onKChange,
}: ParameterPanelProps) {
  return (
    <ControlPanel title="Parameters" className="space-y-4">
      {/* Mode Selection */}
      <div className="space-y-2">
        <label className="text-xs text-muted-foreground">Detection Mode</label>
        <Select
          value={mode}
          onValueChange={(val) => onModeChange(val as CornerDetectionMode)}
          options={[
            { value: "Harris", label: "Harris Corner Detector" },
            { value: "Shi-Tomasi", label: "Shi-Tomasi (λ-)" },
          ]}
        />
      </div>

      {/* Sigma */}
      <div className="space-y-2">
        <div className="flex justify-between">
          <label className="text-xs text-muted-foreground">Sigma</label>
          <span className="text-xs font-mono text-primary">{sigma}</span>
        </div>
        <Slider
          value={[sigma]}
          onValueChange={(val) => onSigmaChange(val[0])}
          min={0.1}
          max={10.0}
          step={0.1}
        />
      </div>

      {/* Window Size */}
      <div className="space-y-2">
        <div className="flex justify-between">
          <label className="text-xs text-muted-foreground">Window Size</label>
          <span className="text-xs font-mono text-primary">{windowSize}</span>
        </div>
        <Slider
          value={[windowSize]}
          onValueChange={(val) => onWindowSizeChange(val[0])}
          min={3}
          max={15}
          step={2}
        />
      </div>

      {/* Threshold */}
      <div className="space-y-2">
        <div className="flex justify-between">
          <label className="text-xs text-muted-foreground">Threshold</label>
          <span className="text-xs font-mono text-primary">{threshold}</span>
        </div>
        <Slider
          value={[threshold]}
          onValueChange={(val) => onThresholdChange(val[0])}
          min={0}
          max={5}
          step={0.1}
        />
      </div>

      {/* K-Factor (Conditional) */}
      {mode === "Harris" && (
        <div className="space-y-2">
          <div className="flex justify-between">
            <label className="text-xs text-muted-foreground">k-Factor</label>
            <span className="text-xs font-mono text-primary">{k}</span>
          </div>
          <Slider
            value={[k]}
            onValueChange={(val) => onKChange(val[0])}
            min={0.01}
            max={0.20}
            step={0.01}
          />
        </div>
      )}
    </ControlPanel>
  );
}
