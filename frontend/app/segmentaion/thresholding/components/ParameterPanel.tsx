"use client";

import { Select } from "@/components/ui/Select";
import { ControlPanel } from "@/components/ControlPanel";

interface ThresholdingParameterPanelProps {
  selectedMethod: string;
  onMethodChange: (method: string) => void;
}

const THRESHOLDING_METHODS = [
  { value: "optimal", label: "Optimal" },
  { value: "otsu", label: "Otsu" },
  { value: "spectral", label: "Spectral" },
  { value: "local", label: "Local" },
];

export function ParameterPanel({
  selectedMethod,
  onMethodChange,
}: ThresholdingParameterPanelProps) {
  return (
    <ControlPanel title="Thresholding Method">
      <div className="space-y-3">
        <div>
          <label className="text-xs text-muted-foreground">Select Method</label>
          <Select
            value={selectedMethod}
            onValueChange={onMethodChange}
            options={THRESHOLDING_METHODS}
            className="mt-2"
          />
        </div>
      </div>
    </ControlPanel>
  );
}
