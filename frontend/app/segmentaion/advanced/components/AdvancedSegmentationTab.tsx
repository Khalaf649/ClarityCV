"use client";

import { useCallback, useState } from "react";
import { useImageContext } from "@/contexts/ImageContext";
import { ImageBox } from "@/components/ImageBox";
import { api } from "@/lib/api";
import {
  ParameterPanel,
  SegmentationMethod,
  SegmentationParams,
} from "./ParametersPanel";
import { ActionButtons } from "./ActionButtons";
import { StatisticsPanel } from "./StatisticsPanel";

const DEFAULT_PARAMS: SegmentationParams = {
  k: 4,
  include_xy: false,
  threshold: 12,
  connectivity: 8,
  clusters: 4,
  sample_size: 32,
  bandwidth: 0.18,
};

// Pick only the params each method actually uses, so unrelated values don't
// leak into the request body and confuse the backend's metadata response.
function paramsForMethod(
  method: SegmentationMethod,
  p: SegmentationParams,
  seed: { x: number; y: number } | null,
): Record<string, unknown> {
  switch (method) {
    case "kmeans":
      return { k: p.k, include_xy: p.include_xy };
    case "region_growing":
      return {
        threshold: p.threshold,
        connectivity: p.connectivity,
        ...(seed ? { seed_x: seed.x, seed_y: seed.y } : {}),
      };
    case "agglomerative":
      return { clusters: p.clusters, sample_size: p.sample_size };
    case "mean_shift":
      return {
        bandwidth: p.bandwidth,
        sample_size: p.sample_size,
      };
  }
}

export default function AdvancedSegmentationTab() {
  const { originalImage, setImageFromFile } = useImageContext();
  const [outputImage, setOutputImage] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [method, setMethod] = useState<SegmentationMethod>("kmeans");
  const [params, setParams] = useState<SegmentationParams>(DEFAULT_PARAMS);
  const [computationTime, setComputationTime] = useState<number | null>(null);
  const [metadata, setMetadata] = useState<Record<string, unknown> | null>(null);
  const [seedPoint, setSeedPoint] =
    useState<{ x: number; y: number } | null>(null);

  const handleApply = useCallback(async () => {
    if (!originalImage) return;
    setLoading(true);
    setError(null);

    const t0 = performance.now();
    try {
      const response = await api.segmentation(
        originalImage,
        method,
        paramsForMethod(method, params, seedPoint),
      );
      const elapsed = (performance.now() - t0) / 1000;
      setOutputImage(`data:${response.mime_type};base64,${response.image_base64}`);
      setMetadata(response.metadata);
      setComputationTime(elapsed);
    } catch (err) {
      const msg = err instanceof Error ? err.message : "Segmentation failed.";
      setError(msg);
    } finally {
      setLoading(false);
    }
  }, [originalImage, method, params, seedPoint]);

  const handleUpload = useCallback(
    async (file: File) => {
      await setImageFromFile(file);
      // New image loaded — old seed point is in the wrong coordinate system.
      setSeedPoint(null);
    },
    [setImageFromFile],
  );

  const handleReset = useCallback(() => {
    setOutputImage(null);
    setMetadata(null);
    setComputationTime(null);
    setMethod("kmeans");
    setParams(DEFAULT_PARAMS);
    setSeedPoint(null);
    setError(null);
  }, []);

  // Image click is wired only when region_growing is active — for other
  // methods the click is meaningless and we want the cursor to stay default.
  const handleImageClick =
    method === "region_growing"
      ? (coords: { x: number; y: number }) => setSeedPoint(coords)
      : undefined;

  return (
    <div className="flex lg:flex-row gap-6 h-full">
      <div className="flex-1 grid grid-cols-2 gap-4">
        <ImageBox
          title="Input"
          image={originalImage}
          onUpload={handleUpload}
          onImageClick={handleImageClick}
          activePoints={seedPoint ? [seedPoint] : []}
        />
        <ImageBox title="Output" image={outputImage} />
      </div>
      <div className="w-full lg:w-72 shrink-0 space-y-4">
        {error && <p className="text-xs text-red-400">{error}</p>}
        {loading && (
          <p className="text-xs text-primary animate-pulse">Processing…</p>
        )}
        <ParameterPanel
          selectedMethod={method}
          onMethodChange={setMethod}
          params={params}
          onParamsChange={setParams}
          seedPoint={seedPoint}
        />
        <ActionButtons
          onApply={handleApply}
          onReset={handleReset}
          applyDisabled={!originalImage}
          loading={loading}
        />
        <StatisticsPanel
          computationTime={computationTime}
          metadata={metadata}
          method={method}
        />
      </div>
    </div>
  );
}
