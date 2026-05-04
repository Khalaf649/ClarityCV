"use client";

import { Select } from "@/components/ui/Select";
import { ControlPanel } from "@/components/ControlPanel";

interface ThresholdingParameterPanelProps {
  selectedMethod: string;
  onMethodChange: (method: string) => void;
}

const THRESHOLDING_METHODS = [
  { value: "kmeans", label: "K-Means" },
  { value: "region_growing", label: "Region Growing" },
  { value: "agglomerative", label: "Agglomerative" },
  { value: "mean_shift", label: "Mean Shift" },
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
