"use client";

import { useCallback, useState } from "react";
import { useImageContext } from "@/contexts/ImageContext";
import { ImageBox } from "@/components/ImageBox";
import { api } from "@/lib/api";
import {
  ParameterPanel,
  ThresholdingMethod,
  ThresholdingParams,
} from "./ParametersPanel";
import { ActionButtons } from "./ActionButtons";
import { StatisticsPanel } from "./StatisticsPanel";

const DEFAULT_PARAMS: ThresholdingParams = {
  epsilon: 0.5,
  max_iter: 100,
  levels: 3,
  window_size: 31,
  offset: 5,
};

// Map each method to the param subset the backend actually expects, so we
// don't send unrelated fields and pollute the metadata response.
function paramsForMethod(
  method: ThresholdingMethod,
  p: ThresholdingParams,
): Record<string, unknown> {
  switch (method) {
    case "optimal":
      return { epsilon: p.epsilon, max_iter: p.max_iter };
    case "otsu":
      return {};
    case "spectral":
      return { levels: p.levels };
    case "local":
      return { window_size: p.window_size, offset: p.offset };
  }
}

export default function ThresholdingTab() {
  const { originalImage, setImageFromFile } = useImageContext();
  const [outputImage, setOutputImage] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [method, setMethod] = useState<ThresholdingMethod>("otsu");
  const [params, setParams] = useState<ThresholdingParams>(DEFAULT_PARAMS);
  const [computationTime, setComputationTime] = useState<number | null>(null);
  const [metadata, setMetadata] = useState<Record<string, unknown> | null>(null);

  const handleApply = useCallback(async () => {
    if (!originalImage) return;
    setLoading(true);
    setError(null);

    // The merged service computes server-side timing internally for face
    // recognition, but `/api/segment` doesn't return a duration. Time it
    // ourselves so the user gets feedback on how long it took.
    const t0 = performance.now();
    try {
      const response = await api.segmentationThresholding(
        originalImage,
        method,
        paramsForMethod(method, params),
      );
      const elapsed = (performance.now() - t0) / 1000;
      // Backend returns a raw base64 PNG (no `data:` prefix).
      setOutputImage(`data:${response.mime_type};base64,${response.image_base64}`);
      setMetadata(response.metadata);
      setComputationTime(elapsed);
    } catch (err) {
      const msg = err instanceof Error ? err.message : "Thresholding failed.";
      setError(msg);
    } finally {
      setLoading(false);
    }
  }, [originalImage, method, params]);

  const handleUpload = useCallback(
    async (file: File) => {
      await setImageFromFile(file);
    },
    [setImageFromFile],
  );

  const handleReset = useCallback(() => {
    setOutputImage(null);
    setMetadata(null);
    setComputationTime(null);
    setMethod("otsu");
    setParams(DEFAULT_PARAMS);
    setError(null);
  }, []);

  return (
    <div className="flex lg:flex-row gap-6 h-full">
      <div className="flex-1 grid grid-cols-2 gap-4">
        <ImageBox title="Input" image={originalImage} onUpload={handleUpload} />
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
