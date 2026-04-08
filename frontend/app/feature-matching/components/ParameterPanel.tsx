"use client";

import { ControlPanel } from "@/components/ControlPanel";
import { Select } from "@/components/ui/Select";
import { Button } from "@/components/ui/Button";

interface ParameterPanelProps {
  selectedMethod: string;
  setSelectedMethod: (v: string) => void;
  runMatching: () => void;
  originalImage: string | null;
  img2: string | null;
  loading: boolean;
}

export function ParameterPanel({
  selectedMethod,
  setSelectedMethod,
  runMatching,
  originalImage,
  img2,
  loading,
}: ParameterPanelProps) {
  return (
    <ControlPanel title="Parameters" className="space-y-4">
      <div className="space-y-2">
        <label className="text-xs text-muted-foreground">Method</label>
        <Select
          value={selectedMethod}
          onValueChange={setSelectedMethod}
          options={[{ value: "SSD", label: "SSD" }, { value: "NCC", label: "NCC" }]}
          className="mt-1"
        />
      </div>

      <Button onClick={runMatching} disabled={!originalImage || !img2 || loading} className="w-full">
        {loading ? "Running…" : "Run Matching"}
      </Button>
    </ControlPanel>
  );
}
