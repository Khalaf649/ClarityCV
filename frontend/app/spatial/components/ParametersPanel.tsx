import { ControlPanel } from "@/components/ControlPanel";
import { Select } from "@/components/ui/Select";
import { Slider } from "@/components/ui/Slider";
import { Button } from "@/components/ui/Button";

interface ParametersPanelProps {
  loading: boolean;
  error: string | null;
  originalImage: string | null;

  // Noise State
  noiseType: string;
  setNoiseType: (v: string) => void;
  noiseOptions: { label: string; value: string }[];
  noisePercent: number;
  setNoisePercent: (v: number) => void;
  applyNoise: () => void;

  // Filter State
  filterType: string;
  setFilterType: (v: string) => void;
  filterOptions: { label: string; value: string }[];
  kernelSize: number;
  setKernelSize: (v: number) => void;
  applyFilter: () => void;

  // Edge State
  edgeType: string;
  setEdgeType: (v: string) => void;
  edgeOptions: { label: string; value: string }[];
  cannyLow: number;
  setCannyLow: (v: number) => void;
  cannyHigh: number;
  setCannyHigh: (v: number) => void;
  applyEdge: () => void;
}

export function ParametersPanel({
  loading,
  error,
  originalImage,
  noiseType,
  setNoiseType,
  noiseOptions,
  noisePercent,
  setNoisePercent,
  applyNoise,
  filterType,
  setFilterType,
  filterOptions,
  kernelSize,
  setKernelSize,
  applyFilter,
  edgeType,
  setEdgeType,
  edgeOptions,
  cannyLow,
  setCannyLow,
  cannyHigh,
  setCannyHigh,
  applyEdge,
}: ParametersPanelProps) {
  return (
    <div className="w-full lg:w-72 shrink-0 space-y-4">
      {error && <p className="text-xs text-red-400">{error}</p>}
      {loading && <p className="text-xs text-primary animate-pulse">Processing…</p>}

      {/* NOISE PANEL */}
      <ControlPanel title="Noise">
        <div className="space-y-3">
          <div>
            <label className="text-xs text-muted-foreground">Type</label>
            <Select value={noiseType} onValueChange={setNoiseType} options={noiseOptions} className="mt-1" />
          </div>
          <div>
            <label className="text-xs text-muted-foreground">Percentage: {noisePercent}%</label>
            <Slider value={[noisePercent]} onValueChange={([v]) => setNoisePercent(v)} min={1} max={100} step={1} className="mt-2" />
          </div>
          <Button onClick={applyNoise} disabled={!originalImage || loading} className="w-full" size="sm">
            Apply Noise
          </Button>
        </div>
      </ControlPanel>

      {/* FILTER PANEL */}
      <ControlPanel title="Filter">
        <div className="space-y-3">
          <div>
            <label className="text-xs text-muted-foreground">Type</label>
            <Select value={filterType} onValueChange={setFilterType} options={filterOptions} className="mt-1" />
          </div>
          <div>
            <label className="text-xs text-muted-foreground">
              Kernel Size: {kernelSize % 2 === 0 ? kernelSize + 1 : kernelSize}
            </label>
            <Slider value={[kernelSize]} onValueChange={([v]) => setKernelSize(v)} min={3} max={15} step={2} className="mt-2" />
          </div>
          <Button onClick={applyFilter} disabled={!originalImage || loading} className="w-full" size="sm">
            Apply Filter
          </Button>
        </div>
      </ControlPanel>

      {/* EDGE DETECTION PANEL */}
      <ControlPanel title="Edge Detection">
        <div className="space-y-3">
          <div>
            <label className="text-xs text-muted-foreground">Detector</label>
            <Select value={edgeType} onValueChange={setEdgeType} options={edgeOptions} className="mt-1" />
          </div>
          {edgeType === "canny" && (
            <>
              <div>
                <label className="text-xs text-muted-foreground">Low Threshold: {cannyLow}</label>
                <Slider value={[cannyLow]} onValueChange={([v]) => setCannyLow(v)} min={1} max={200} step={1} className="mt-2" />
              </div>
              <div>
                <label className="text-xs text-muted-foreground">High Threshold: {cannyHigh}</label>
                <Slider value={[cannyHigh]} onValueChange={([v]) => setCannyHigh(v)} min={1} max={400} step={1} className="mt-2" />
              </div>
            </>
          )}
          <Button onClick={applyEdge} disabled={!originalImage || loading} className="w-full" size="sm">
            Detect Edges
          </Button>
        </div>
      </ControlPanel>
    </div>
  );
}