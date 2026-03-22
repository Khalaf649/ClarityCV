"use client";

import { useState, useCallback, useEffect } from "react";
import { ImageBox } from "@/components/ImageBox";
import { ParametersPanel } from "./ParametersPanel";
import { useImageContext } from "@/contexts/ImageContext";
import { api } from "@/lib/api";

const noiseOptions = [
  { value: "gaussian", label: "Gaussian" },
  { value: "uniform", label: "Uniform" },
  { value: "salt_pepper", label: "Salt & Pepper" },
];

const filterOptions = [
  { value: "average", label: "Average" },
  { value: "gaussian", label: "Gaussian" },
  { value: "median", label: "Median" },
];

const edgeOptions = [
  { value: "sobel", label: "Sobel" },
  { value: "roberts", label: "Roberts" },
  { value: "prewitt", label: "Prewitt" },
  { value: "canny", label: "Canny" },
];

export function SpatialFilterTab() {
  const { originalImage, setImageFromFile } = useImageContext();

  const [noisy, setNoisy] = useState<string | null>(null);
  const [filtered, setFiltered] = useState<string | null>(null);
  const [edgeX, setEdgeX] = useState<string | null>(null);
  const [edgeY, setEdgeY] = useState<string | null>(null);
  const [edgeCombined, setEdgeCombined] = useState<string | null>(null);

  const [noiseType, setNoiseType] = useState("gaussian");
  const [noisePercent, setNoisePercent] = useState(20);
  const [filterType, setFilterType] = useState("average");
  const [kernelSize, setKernelSize] = useState(3);
  const [edgeType, setEdgeType] = useState("sobel");
  const [cannyLow, setCannyLow] = useState(50);
  const [cannyHigh, setCannyHigh] = useState(150);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

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
        stddev: sigma,
        mean: 0,
        low: -amount / 2,
        high: amount / 2,
        salt_prob: prob / 2,
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
        canny_low: cannyLow,
        canny_high: cannyHigh,
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
        <ImageBox title="Original" image={originalImage} onUpload={handleUpload} />
        <ImageBox title="After Noise" image={noisy} />
        <ImageBox title="After Filter" image={filtered} />
        <ImageBox title="Edge X" image={edgeX} />
        <ImageBox title="Edge Y" image={edgeY} />
        <ImageBox title="Edge Combined" image={edgeCombined} />
      </div>

      {/* Controls */}
      <div className="w-full lg:w-72 shrink-0 space-y-4">
        {error && <p className="text-xs text-red-400">{error}</p>}
        {loading && <p className="text-xs text-primary animate-pulse">Processing…</p>}

        <ParametersPanel
          loading={loading}
          error={error}
          originalImage={originalImage}
          // Noise
          noiseType={noiseType} setNoiseType={setNoiseType} noiseOptions={noiseOptions}
          noisePercent={noisePercent} setNoisePercent={setNoisePercent} applyNoise={applyNoise}
          // Filter
          filterType={filterType} setFilterType={setFilterType} filterOptions={filterOptions}
          kernelSize={kernelSize} setKernelSize={setKernelSize} applyFilter={applyFilter}
          // Edge
          edgeType={edgeType} setEdgeType={setEdgeType} edgeOptions={edgeOptions}
          cannyLow={cannyLow} setCannyLow={setCannyLow} cannyHigh={cannyHigh} setCannyHigh={setCannyHigh}
          applyEdge={applyEdge}
        />
      </div>
    </div>
  );
}
