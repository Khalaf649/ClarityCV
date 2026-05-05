"use client";

import { Select } from "@/components/ui/Select";
import { ControlPanel } from "@/components/ControlPanel";
import { Slider } from "@/components/ui/Slider";

export type ThresholdingMethod = "optimal" | "otsu" | "spectral" | "local";

export interface ThresholdingParams {
  // optimal
  epsilon: number;
  max_iter: number;
  // spectral
  levels: number;
  // local
  window_size: number;
  offset: number;
}

interface ParameterPanelProps {
  selectedMethod: ThresholdingMethod;
  onMethodChange: (m: ThresholdingMethod) => void;
  params: ThresholdingParams;
  onParamsChange: (p: ThresholdingParams) => void;
}

const THRESHOLDING_METHODS: Array<{ value: ThresholdingMethod; label: string }> = [
  { value: "optimal", label: "Optimal (iterative)" },
  { value: "otsu", label: "Otsu" },
  { value: "spectral", label: "Spectral (multi-level)" },
  { value: "local", label: "Local (adaptive)" },
];

export function ParameterPanel({
  selectedMethod,
  onMethodChange,
  params,
  onParamsChange,
}: ParameterPanelProps) {
  // Helper to update one field of params without losing the others.
  const set = <K extends keyof ThresholdingParams>(
    key: K,
    value: ThresholdingParams[K],
  ) => onParamsChange({ ...params, [key]: value });

  return (
    <ControlPanel title="Thresholding">
      <div className="space-y-3">
        <div>
          <label className="text-xs text-muted-foreground">Select Method</label>
          <Select
            value={selectedMethod}
            onValueChange={(v) => onMethodChange(v as ThresholdingMethod)}
            options={THRESHOLDING_METHODS}
            className="mt-2"
          />
        </div>

        {/* Method-specific parameters. Otsu has no tunables. */}

        {selectedMethod === "optimal" && (
          <>
            <div>
              <label className="text-xs text-muted-foreground">
                Epsilon (convergence): {params.epsilon.toFixed(2)}
              </label>
              <Slider
                value={[params.epsilon]}
                onValueChange={([v]) => set("epsilon", v)}
                min={0.01}
                max={5}
                step={0.05}
                className="mt-2"
              />
            </div>
            <div>
              <label className="text-xs text-muted-foreground">
                Max iterations: {params.max_iter}
              </label>
              <Slider
                value={[params.max_iter]}
                onValueChange={([v]) => set("max_iter", Math.round(v))}
                min={10}
                max={500}
                step={10}
                className="mt-2"
              />
            </div>
          </>
        )}

        {selectedMethod === "spectral" && (
          <div>
            <label className="text-xs text-muted-foreground">
              Levels: {params.levels}
            </label>
            <Slider
              value={[params.levels]}
              onValueChange={([v]) => set("levels", Math.round(v))}
              min={3}
              max={8}
              step={1}
              className="mt-2"
            />
          </div>
        )}

        {selectedMethod === "local" && (
          <>
            <div>
              <label className="text-xs text-muted-foreground">
                Window size: {params.window_size}
              </label>
              <Slider
                value={[params.window_size]}
                onValueChange={([v]) => {
                  // Window size must be odd for symmetric neighborhoods.
                  const odd = Math.round(v) | 1;
                  set("window_size", odd);
                }}
                min={3}
                max={101}
                step={2}
                className="mt-2"
              />
            </div>
            <div>
              <label className="text-xs text-muted-foreground">
                Offset: {params.offset.toFixed(2)}
              </label>
              <Slider
                value={[params.offset]}
                onValueChange={([v]) => set("offset", v)}
                min={-20}
                max={20}
                step={0.5}
                className="mt-2"
              />
            </div>
          </>
        )}

        {selectedMethod === "otsu" && (
          <p className="text-xs text-muted-foreground italic">
            Otsu has no tunable parameters — it picks the threshold that
            maximises between-class variance from the image histogram.
          </p>
        )}
      </div>
    </ControlPanel>
  );
}
