"use client";
import { Select } from "@/components/ui/Select";
import { ControlPanel } from "@/components/ControlPanel";
import { Slider } from "@/components/ui/Slider";
interface ThresholdingParameterPanelProps {
  selectedMethod: string;
  onMethodChange: (method: string) => void;
  selectedThreshold: number;
  onThresholdChange: (threshold: number) => void;
}
const THRESHOLDING_METHODS = [
  { value: "face_recognition", label: "Face Recognition" },
  { value: "haar_detection", label: "Face Detection" },
];
export function ParameterPanel({
  selectedMethod,
  selectedThreshold,
  onMethodChange,
  onThresholdChange,
}: ThresholdingParameterPanelProps) {
  return (
    <ControlPanel
      title={
        selectedMethod === "face_recognition"
          ? "Face Recognition"
          : "Face Detection"
      }
    >
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

        <div>
          <label className="text-xs text-muted-foreground">
            Threshold: {selectedThreshold.toFixed(2)}
          </label>
          <Slider
            value={[selectedThreshold]}
            onValueChange={([v]) => onThresholdChange(v)}
            min={1}
            max={10}
            step={0.5}
            className="mt-2"
          />
        </div>
      </div>
    </ControlPanel>
  );
}
