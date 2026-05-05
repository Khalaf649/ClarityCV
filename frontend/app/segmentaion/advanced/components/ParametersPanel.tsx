"use client";

import { Select } from "@/components/ui/Select";
import { ControlPanel } from "@/components/ControlPanel";
import { Slider } from "@/components/ui/Slider";

export type SegmentationMethod =
  | "kmeans"
  | "region_growing"
  | "agglomerative"
  | "mean_shift";

export interface SegmentationParams {
  // kmeans
  k: number;
  include_xy: boolean;
  // region_growing
  threshold: number;
  connectivity: number;
  // agglomerative
  clusters: number;
  sample_size: number;
  // mean_shift
  bandwidth: number;
}

interface ParameterPanelProps {
  selectedMethod: SegmentationMethod;
  onMethodChange: (m: SegmentationMethod) => void;
  params: SegmentationParams;
  onParamsChange: (p: SegmentationParams) => void;
  // Region-growing seed point (clicked on the input image).
  seedPoint: { x: number; y: number } | null;
}

const METHODS: Array<{ value: SegmentationMethod; label: string }> = [
  { value: "kmeans", label: "K-means" },
  { value: "region_growing", label: "Region Growing" },
  { value: "agglomerative", label: "Agglomerative" },
  { value: "mean_shift", label: "Mean Shift" },
];

export function ParameterPanel({
  selectedMethod,
  onMethodChange,
  params,
  onParamsChange,
  seedPoint,
}: ParameterPanelProps) {
  const set = <K extends keyof SegmentationParams>(
    key: K,
    value: SegmentationParams[K],
  ) => onParamsChange({ ...params, [key]: value });

  return (
    <ControlPanel title="Advanced Segmentation">
      <div className="space-y-3">
        <div>
          <label className="text-xs text-muted-foreground">Select Method</label>
          <Select
            value={selectedMethod}
            onValueChange={(v) => onMethodChange(v as SegmentationMethod)}
            options={METHODS}
            className="mt-2"
          />
        </div>

        {selectedMethod === "kmeans" && (
          <>
            <div>
              <label className="text-xs text-muted-foreground">
                Clusters (k): {params.k}
              </label>
              <Slider
                value={[params.k]}
                onValueChange={([v]) => set("k", Math.round(v))}
                min={2}
                max={10}
                step={1}
                className="mt-2"
              />
            </div>
            <div>
              <label className="text-xs text-muted-foreground flex items-center gap-2">
                <input
                  type="checkbox"
                  checked={params.include_xy}
                  onChange={(e) => set("include_xy", e.target.checked)}
                  className="accent-cyan-400"
                />
                Include xy (spatial features)
              </label>
              <p className="text-[10px] text-muted-foreground mt-1">
                When on, pixel position is added to color so spatially-adjacent
                pixels are more likely to share a cluster.
              </p>
            </div>
          </>
        )}

        {selectedMethod === "region_growing" && (
          <>
            <p className="text-xs text-muted-foreground italic">
              Click on the input image to set the seed point.
              {seedPoint && (
                <span className="block mt-1 text-cyan-400 not-italic">
                  Seed: ({seedPoint.x}, {seedPoint.y})
                </span>
              )}
              {!seedPoint && (
                <span className="block mt-1 text-amber-400 not-italic">
                  No seed yet — defaulting to image centre.
                </span>
              )}
            </p>
            <div>
              <label className="text-xs text-muted-foreground">
                Intensity threshold: {params.threshold.toFixed(1)}
              </label>
              <Slider
                value={[params.threshold]}
                onValueChange={([v]) => set("threshold", v)}
                min={1}
                max={50}
                step={1}
                className="mt-2"
              />
            </div>
            <div>
              <label className="text-xs text-muted-foreground">
                Connectivity: {params.connectivity}
              </label>
              <Select
                value={String(params.connectivity)}
                onValueChange={(v) => set("connectivity", Number(v))}
                options={[
                  { value: "4", label: "4-connected" },
                  { value: "8", label: "8-connected" },
                ]}
                className="mt-2"
              />
            </div>
          </>
        )}

        {selectedMethod === "agglomerative" && (
          <>
            <div>
              <label className="text-xs text-muted-foreground">
                Clusters: {params.clusters}
              </label>
              <Slider
                value={[params.clusters]}
                onValueChange={([v]) => set("clusters", Math.round(v))}
                min={2}
                max={10}
                step={1}
                className="mt-2"
              />
            </div>
            <div>
              <label className="text-xs text-muted-foreground">
                Sample size: {params.sample_size}
              </label>
              <Slider
                value={[params.sample_size]}
                onValueChange={([v]) => set("sample_size", Math.round(v))}
                min={16}
                max={64}
                step={4}
                className="mt-2"
              />
              <p className="text-[10px] text-muted-foreground mt-1">
                Agglomerative is O(n²) — sample size caps the work. Higher =
                slower, slightly better palette.
              </p>
            </div>
          </>
        )}

        {selectedMethod === "mean_shift" && (
          <>
            <div>
              <label className="text-xs text-muted-foreground">
                Bandwidth: {params.bandwidth.toFixed(2)}
              </label>
              <Slider
                value={[params.bandwidth]}
                onValueChange={([v]) => set("bandwidth", v)}
                min={0.05}
                max={0.5}
                step={0.01}
                className="mt-2"
              />
              <p className="text-[10px] text-muted-foreground mt-1">
                Smaller bandwidth → more clusters; larger → fewer.
              </p>
            </div>
            <div>
              <label className="text-xs text-muted-foreground">
                Sample size: {params.sample_size}
              </label>
              <Slider
                value={[params.sample_size]}
                onValueChange={([v]) => set("sample_size", Math.round(v))}
                min={16}
                max={96}
                step={8}
                className="mt-2"
              />
            </div>
          </>
        )}
      </div>
    </ControlPanel>
  );
}
