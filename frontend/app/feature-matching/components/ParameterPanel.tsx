"use client";

import { ControlPanel } from "@/components/ControlPanel";
import { Select }       from "@/components/ui/Select";
import { Slider }       from "@/components/ui/Slider";
import { Button }       from "@/components/ui/Button";

interface ParameterPanelProps {
  selectedMethod:    string;
  setSelectedMethod: (v: string) => void;
  maxMatches:        number;
  setMaxMatches:     (v: number) => void;
  ratioThreshold:    number;
  setRatioThreshold: (v: number) => void;
  runMatching:       () => void;
  originalImage:     string | null;
  img2:              string | null;
  loading:           boolean;
}

export function ParameterPanel({
  selectedMethod,
  setSelectedMethod,
  maxMatches,
  setMaxMatches,
  ratioThreshold,
  setRatioThreshold,
  runMatching,
  originalImage,
  img2,
  loading,
}: ParameterPanelProps) {
  return (
    <ControlPanel title="Parameters" className="space-y-4">

      {/* Matching method */}
      <div className="space-y-2">
        <label className="text-xs text-muted-foreground">Method</label>
        <Select
          value={selectedMethod}
          onValueChange={setSelectedMethod}
          options={[
            { value: "SSD", label: "SSD – Sum of Squared Differences" },
            { value: "NCC", label: "NCC – Normalised Cross-Correlation" },
          ]}
          className="mt-1"
        />
      </div>

      {/* Max matches */}
      <div className="space-y-2">
        <label className="text-xs text-muted-foreground">
          Max Matches:{" "}
          <span className="text-primary font-mono">{maxMatches}</span>
        </label>
        <Slider
          min={5}
          max={200}
          step={5}
          value={[maxMatches]}
          onValueChange={(val) => setMaxMatches(val[0])}
        />
      </div>

      {/* Lowe's ratio threshold */}
      <div className="space-y-2">
        <label className="text-xs text-muted-foreground">
          Ratio Threshold:{" "}
          <span className="text-primary font-mono">{ratioThreshold.toFixed(2)}</span>
        </label>
        <Slider
          min={0.5}
          max={0.95}
          step={0.01}
          value={[ratioThreshold]}
          onValueChange={(val) => setRatioThreshold(val[0])}
        />
        <p className="text-[10px] text-muted-foreground leading-tight">
          Lowe&apos;s ratio test — lower = stricter, fewer but more reliable matches.
        </p>
      </div>

      <Button
        onClick={runMatching}
        disabled={!originalImage || !img2 || loading}
        className="w-full"
      >
        {loading ? "Running…" : "Run Matching"}
      </Button>
    </ControlPanel>
  );
}
