"use client";

import { useCallback, useState } from "react";
import { useImageContext } from "@/contexts/ImageContext";
import { ImageBox } from "@/components/ImageBox";
import { ParameterPanel } from "./components/ParameterPanel";
import { ActionButtons } from "../components/ActionButtons";
import { api } from "@/lib/api";

export default function ThresholdingPage() {
  const { originalImage, setImageFromFile } = useImageContext();
  const [selectedMethod, setSelectedMethod] = useState("kmeans");
  const [outputImage, setOutputImage] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const handleUpload = useCallback(
    async (file: File) => {
      await setImageFromFile(file);
    },
    [setImageFromFile],
  );

  const handleApply = async () => {
    if (!originalImage) return;

    setLoading(true);
    try {
      const data = await api.segmentationThresholding(
        originalImage,
        selectedMethod as
          | "kmeans"
          | "region_growing"
          | "agglomerative"
          | "mean_shift",
      );
      setOutputImage(data.result);
    } catch (error) {
      console.error("Error applying thresholding:", error);
      setError("Error applying thresholding. Please try again.");
    } finally {
      setLoading(false);
    }
  };

  const handleReset = () => {
    setOutputImage(null);
    setSelectedMethod("kmeans");
  };

  return (
    <div className="flex lg:flex-row gap-6 h-full">
      <div className="flex-1 grid grid-cols-2 gap-4">
        <ImageBox title="Input" image={originalImage} onUpload={handleUpload} />
        <ImageBox title="Output" image={outputImage} />
      </div>
      {/* Controls */}
      <div className="w-full lg:w-72 shrink-0 space-y-4">
        {error && <p className="text-xs text-red-400">{error}</p>}
        {loading && (
          <p className="text-xs text-primary animate-pulse">Processing…</p>
        )}

        <ParameterPanel
          selectedMethod={selectedMethod}
          onMethodChange={setSelectedMethod}
        />

        <ActionButtons
          onApply={handleApply}
          onReset={handleReset}
          applyDisabled={!originalImage}
          loading={loading}
        />
      </div>
    </div>
  );
}
