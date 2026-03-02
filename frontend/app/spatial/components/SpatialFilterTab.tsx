"use client";

import { useState, useCallback, useEffect } from "react";
import { ImageBox } from "@/components/ImageBox";
import { Slider } from "@/components/ui/Slider";
import { Select } from "@/components/ui/Select";
import { Button } from "@/components/ui/Button";
import { ControlPanel } from "@/components/ControlPanel";
import { useImageContext } from "@/contexts/ImageContext";
import { api } from "@/lib/api";

const noiseOptions = [
  { value: "gaussian",    label: "Gaussian" },
  { value: "uniform",     label: "Uniform" },
  { value: "salt_pepper", label: "Salt & Pepper" },
];

const filterOptions = [
  { value: "average",  label: "Average" },
  { value: "gaussian", label: "Gaussian" },
  { value: "median",   label: "Median" },
];

const edgeOptions = [
  { value: "sobel",   label: "Sobel" },
  { value: "roberts", label: "Roberts" },
  { value: "prewitt", label: "Prewitt" },
  { value: "canny",   label: "Canny" },
];

export function SpatialFilterTab() {
  const { originalImage, setImageFromFile } = useImageContext();

  const [noisy,    setNoisy]    = useState<string | null>(null);
  const [filtered, setFiltered] = useState<string | null>(null);
  const [edgeX,    setEdgeX]    = useState<string | null>(null);
  const [edgeY,    setEdgeY]    = useState<string | null>(null);
  const [edgeCombined, setEdgeCombined] = useState<string | null>(null);

  const [noiseType,    setNoiseType]    = useState("gaussian");
  const [noisePercent, setNoisePercent] = useState(20);
  const [filterType,   setFilterType]   = useState("average");
  const [kernelSize,   setKernelSize]   = useState(3);
  const [edgeType,     setEdgeType]     = useState("sobel");
  const [cannyLow,     setCannyLow]     = useState(50);
  const [cannyHigh,    setCannyHigh]    = useState(150);
  const [loading,      setLoading]      = useState(false);
  const [error,        setError]        = useState<string | null>(null);

  useEffect(() => {
    setNoisy(null);
    setFiltered(null);
    setEdgeX(null);
    setEdgeY(null);
    setEdgeCombined(null);
  }, [originalImage]);

  const handleUpload = useCallback(
    async (file: File) => { await setImageFromFile(file); },
    [setImageFromFile],
  );

  const applyNoise = async () => {
    if (!originalImage) return;
    setLoading(true); setError(null);
    try {
      const sigma = (noisePercent / 100) * 80;
      const amount = (noisePercent / 100) * 255;
      const prob = noisePercent / 100;

      const res = await api.noise(originalImage, noiseType as "gaussian" | "uniform" | "salt_pepper", {
        stddev:      sigma,
        mean:        0,
        low:         -amount / 2,
        high:        amount / 2,
        salt_prob:   prob / 2,
        pepper_prob: prob / 2,
      });
      setNoisy(res.image);
      setFiltered(null);
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : "Error");
    } finally {
      setLoading(false);
    }
  };

  const applyFilter = async () => {
    const src = noisy ?? originalImage;
    if (!src) return;
    setLoading(true); setError(null);
    try {
      const k = kernelSize % 2 === 0 ? kernelSize + 1 : kernelSize;
      const res = await api.filter(src, filterType as "average" | "gaussian" | "median", k);
      setFiltered(res.image);
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : "Error");
    } finally {
      setLoading(false);
    }
  };

  const applyEdge = async () => {
    const src = originalImage;
    if (!src) return;
    setLoading(true); setError(null);
    try {
      const res = await api.edge(src, edgeType as "sobel" | "roberts" | "prewitt" | "canny", {
        canny_low:   cannyLow,
        canny_high:  cannyHigh,
        sobel_ksize: 3,
      });
      setEdgeX(res.image_x);
      setEdgeY(res.image_y);
      setEdgeCombined(res.image_combined);
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : "Error");
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="flex flex-col lg:flex-row gap-6 h-full">
      {/* Images */}
      <div className="flex-1 grid grid-cols-2 md:grid-cols-3 gap-4">
        <ImageBox title="Original"       image={originalImage} onUpload={handleUpload} />
        <ImageBox title="After Noise"    image={noisy} />
        <ImageBox title="After Filter"   image={filtered} />
        <ImageBox title="Edge X"         image={edgeX} />
        <ImageBox title="Edge Y"         image={edgeY} />
        <ImageBox title="Edge Combined"  image={edgeCombined} />
      </div>

      {/* Controls */}
      <div className="w-full lg:w-72 shrink-0 space-y-4">
        {error && <p className="text-xs text-red-400">{error}</p>}
        {loading && <p className="text-xs text-primary animate-pulse">Processing…</p>}

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
    </div>
  );
}
