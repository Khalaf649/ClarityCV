import { ControlPanel } from "@/components/ControlPanel";
import { Select } from "@/components/ui/Select";
import { Slider } from "@/components/ui/Slider";

interface ParametersPanelProps {
  filterType: "low_pass" | "high_pass";
  setFilterType: (v: "low_pass" | "high_pass") => void;
  filterTypeOptions: { label: string; value: string }[];
  cutoff: number;
  setCutoff: (v: number) => void;
  loading: boolean;
  error: string | null;
}

export function ParametersPanel({
  filterType,
  setFilterType,
  filterTypeOptions,
  cutoff,
  setCutoff,
  loading,
  error
}: ParametersPanelProps) {
  return (
    <div className="w-full lg:w-72 shrink-0 space-y-4">
      {error && <p className="text-xs text-red-400">{error}</p>}
      {loading && <p className="text-xs text-primary animate-pulse">Processing…</p>}

      <ControlPanel title="Frequency Filter">
        <div className="space-y-3">
          <div>
            <label className="text-xs text-muted-foreground">Filter Type</label>
            <Select
              value={filterType}
              onValueChange={(v) => setFilterType(v as "low_pass" | "high_pass")}
              options={filterTypeOptions}
              className="mt-1"
            />
          </div>
          <div>
            <label className="text-xs text-muted-foreground">Cutoff: {cutoff}</label>
            <Slider 
              value={[cutoff]} 
              onValueChange={([v]) => setCutoff(v)} 
              min={1} 
              max={200} 
              step={1} 
              className="mt-2" 
            />
          </div>
        </div>
      </ControlPanel>
    </div>
  );
}