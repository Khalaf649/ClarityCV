"use client";

import { ControlPanel } from "@/components/ControlPanel";
import { Slider } from "@/components/ui/Slider";
import { Button } from "@/components/ui/Button";

interface ParameterPanelProps {
  contrastThreshold: number;
  setContrastThreshold: (v: number) => void;
  nfeatures: number;
  setNfeatures: (v: number) => void;
  runSIFT: () => void;
  originalImage: string | null;
  isProcessing: boolean;
}

export function ParameterPanel({
  contrastThreshold,
  setContrastThreshold,
  nfeatures,
  setNfeatures,
  runSIFT,
  originalImage,
  isProcessing,
}: ParameterPanelProps) {
  return (
    <ControlPanel title="Parameters" className="space-y-4">
      <div className="space-y-2">
        <div className="flex justify-between">
          <label className="text-xs text-muted-foreground">N Features</label>
          <span className="text-xs font-mono text-primary">{nfeatures}</span>
        </div>
        <Slider
          value={[nfeatures]}
          onValueChange={(v) => setNfeatures(v[0])}
          min={100}
          max={2000}
          step={50}
        />
      </div>

      <Button onClick={runSIFT} disabled={!originalImage || isProcessing} className="w-full">
        {isProcessing ? "Running…" : "Run SIFT"}
      </Button>
    </ControlPanel>
  );
}
