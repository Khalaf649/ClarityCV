"use client";

import { useCallback, useEffect, useState } from "react";
import { ImageBox } from "@/components/ImageBox";
import { useImageContext } from "@/contexts/ImageContext";
import { ParameterPanel } from "./ParameterPanel";
import { StatisticsPanel } from "./StatisticsPanel";
import { Button } from "@/components/ui/Button";
import { api } from "@/lib/api";

type CornerDetectionMode = "Harris" | "Shi-Tomasi";

export function CornerDetectionTab() {
  const { originalImage, setImageFromFile } = useImageContext();
  
  const [outputImage, setOutputImage] = useState<string | null>(null);
  const [computationTime, setComputationTime] = useState<number | null>(null);
  const [featureCount, setFeatureCount] = useState<number | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const [mode, setMode] = useState<CornerDetectionMode>("Harris");
  const [sigma, setSigma] = useState(1.0);
  const [windowSize, setWindowSize] = useState(3);
  const [threshold, setThreshold] = useState(100);
  const [k, setK] = useState(0.04);

  // Clear output when original image changes
  useEffect(() => {
    setOutputImage(null);
    setComputationTime(null);
    setFeatureCount(null);
  }, [originalImage]);

  const handleUpload = useCallback(
    async (file: File) => {
      await setImageFromFile(file);
    },
    [setImageFromFile]
  );

  const handleApply = async () => {
    if (!originalImage) {
      setError("Please upload an image first.");
      return;
    }

    setLoading(true);
    setError(null);
    
    // Calling the real backend endpoint
    try {
      const result = await api.cornerDetection(originalImage, mode, sigma, windowSize, threshold, k);
      setOutputImage(result.image);
      setComputationTime(result.computationTime);
      setFeatureCount(result.featureCount);
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : "An error occurred");
    } finally {
      setLoading(false);
    }
  };

  const handleReset = () => {
    setOutputImage(null);
    setComputationTime(null);
    setFeatureCount(null);
    setError(null);
    
    setMode("Harris");
    setSigma(1.0);
    setWindowSize(3);
    setThreshold(100);
    setK(0.04);
  };

  return (
    <div className="flex flex-col lg:flex-row gap-6 h-full">
      {/* Images Section */}
      <div className="flex-1 grid grid-cols-2 gap-4">
        <ImageBox
          title="Input"
          image={originalImage}
          onUpload={handleUpload}
        />
        <ImageBox 
          title="Output" 
          image={outputImage} 
        />
      </div>

      {/* Controls Section */}
      <div className="w-full lg:w-72 shrink-0 space-y-4">
        {error && <p className="text-xs text-red-400">{error}</p>}
        {loading && <p className="text-xs text-primary animate-pulse">Processing…</p>}

        <ParameterPanel
          mode={mode}
          sigma={sigma}
          windowSize={windowSize}
          threshold={threshold}
          k={k}
          onModeChange={setMode}
          onSigmaChange={setSigma}
          onWindowSizeChange={setWindowSize}
          onThresholdChange={setThreshold}
          onKChange={setK}
        />

        <StatisticsPanel
          computationTime={computationTime}
          featureCount={featureCount}
        />

        {/* Action Buttons */}
        <div className="flex flex-col gap-2 pt-2">
          <Button
            onClick={handleApply}
            disabled={!originalImage || loading}
            className="w-full"
          >
            {loading ? "Processing..." : "Apply Detection"}
          </Button>
          <Button 
            onClick={handleReset} 
            variant="outline" 
            className="w-full"
            disabled={loading}
          >
            Reset
          </Button>
        </div>
      </div>
    </div>
  );
}
