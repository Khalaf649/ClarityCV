"use client";

import { Slider } from "@/components/ui/Slider";
import { ControlPanel } from "@/components/ControlPanel";

interface ParametersPanelProps {
  alpha: number;
  beta: number;
  gamma: number;
  iterations: number;
  onAlphaChange: (value: number) => void;
  onBetaChange: (value: number) => void;
  onGammaChange: (value: number) => void;
  onIterationsChange: (value: number) => void;
}

export function ParametersPanel({
  alpha,
  beta,
  gamma,
  iterations,
  onAlphaChange,
  onBetaChange,
  onGammaChange,
  onIterationsChange,
}: ParametersPanelProps) {
  return (
    <ControlPanel title="Parameters">
      <div className="space-y-3">
        <div>
          <label className="text-xs text-muted-foreground">Alpha: {alpha.toFixed(2)}</label>
          <Slider
            value={[alpha]}
            onValueChange={([v]) => onAlphaChange(v)}
            min={0}
            max={5}
            step={0.1}
            className="mt-2"
          />
        </div>

        <div>
          <label className="text-xs text-muted-foreground">Beta: {beta.toFixed(2)}</label>
          <Slider
            value={[beta]}
            onValueChange={([v]) => onBetaChange(v)}
            min={0}
            max={5}
            step={0.1}
            className="mt-2"
          />
        </div>

        <div>
          <label className="text-xs text-muted-foreground">Gamma: {gamma.toFixed(2)}</label>
          <Slider
            value={[gamma]}
            onValueChange={([v]) => onGammaChange(v)}
            min={0}
            max={5}
            step={0.1}
            className="mt-2"
          />
        </div>

        <div>
          <label className="text-xs text-muted-foreground">Iterations: {iterations}</label>
          <Slider
            value={[iterations]}
            onValueChange={([v]) => onIterationsChange(v)}
            min={1}
            max={500}
            step={1}
            className="mt-2"
          />
        </div>
      </div>
    </ControlPanel>
  );
}
