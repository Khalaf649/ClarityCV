import React from "react";
import { ControlPanel } from "@/components/ControlPanel";
import { Select } from "@/components/ui/Select";
import { Slider } from "@/components/ui/Slider";
import { Button } from  "@/components/ui/Button";

interface ParametersPanelProps {
  shapeType: string;
  setShapeType: (v: string) => void;
  shapeOptions: { label: string; value: string }[];
  votesThreshold: number;
  setVotesThreshold: (v: number) => void;
  applyHough: () => void;
  originalImage: string | null;
  loading: boolean;
  error?: string | null;
}

export function ParametersPanel({
  shapeType,
  setShapeType,
  shapeOptions,
  votesThreshold,
  setVotesThreshold,
  applyHough,
  originalImage,
  loading,
  error
}: ParametersPanelProps) {
  return (
    <div className="w-full lg:w-72 shrink-0 space-y-4">
      {error && <p className="text-xs text-red-400">{error}</p>}
      {loading && <p className="text-xs text-primary animate-pulse">Processing…</p>}

      <ControlPanel title="Hough Transform">
        <div className="space-y-3">
          <div>
            <label className="text-xs text-muted-foreground">Shape Type</label>
            <Select 
              value={shapeType} 
              onValueChange={setShapeType} 
              options={shapeOptions} 
              className="mt-1" 
            />
          </div>
          <div>
            <label className="text-xs text-muted-foreground">Votes Threshold: {votesThreshold}</label>
            <Slider 
              value={[votesThreshold]} 
              onValueChange={([v]) => setVotesThreshold(v)} 
              min={1} 
              max={300} 
              step={1} 
              className="mt-2" 
            />
          </div>
          <Button 
            onClick={applyHough} 
            disabled={!originalImage || loading} 
            className="w-full" 
            size="sm"
          >
            Apply
          </Button>
        </div>
      </ControlPanel>
    </div>
  );
}