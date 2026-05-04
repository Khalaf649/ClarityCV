"use client";

import { Select } from "@/components/ui/Select";
import { ControlPanel } from "@/components/ControlPanel";

interface AdvancedParameterPanelProps {
  selectedMethod: string;
  onMethodChange: (method: string) => void;
}

const ADVANCED_METHODS = [
  { value: "kmeans", label: "K-Means" },
  { value: "region_growing", label: "Region Growing" },
  { value: "agglomerative", label: "Agglomerative" },
  { value: "mean_shift", label: "Mean Shift" },
];

export function ParameterPanel({
  selectedMethod,
  onMethodChange,
}: AdvancedParameterPanelProps) {
  return (
    <ControlPanel title="Segmentation Method">
      <div className="space-y-3">
        <div>
          <label className="text-xs text-muted-foreground">Select Method</label>
          <Select
            value={selectedMethod}
            onValueChange={onMethodChange}
            options={ADVANCED_METHODS}
            className="mt-2"
          />
        </div>
      </div>
    </ControlPanel>
  );
}
