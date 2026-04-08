"use client";

import { useState, useCallback, useEffect } from "react";
import { ImageBox } from "@/components/ImageBox";
import { ControlPanel } from "@/components/ControlPanel";
import { useImageContext } from "@/contexts/ImageContext";
import { ParameterPanel } from "./ParameterPanel";
import { StatisticsPanel } from "./StatisticsPanel";
import { api } from "@/lib/api";

export function SIFTTab() {
  const { originalImage, setImageFromFile } = useImageContext();

  const [outputImage, setOutputImage] = useState<string | null>(null);
  const [computationTime, setComputationTime] = useState<number | null>(null);
  const [keypointsCount, setKeypointsCount] = useState<number | null>(null);

  const [contrastThreshold, setContrastThreshold] = useState<number>(0.04);
  const [nfeatures, setNfeatures] = useState<number>(500);
  const [isProcessing, setIsProcessing] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    setOutputImage(null);
    setComputationTime(null);
    setKeypointsCount(null);
  }, [originalImage]);

  const handleUpload = useCallback(
    async (file: File) => {
      await setImageFromFile(file);
    },
    [setImageFromFile],
  );

  const runSIFT = async () => {
    if (!originalImage) {
      setError("Please upload an image first.");
      return;
    }

    setIsProcessing(true);
    setError(null);

    try {
      // If backend provides an endpoint, call it. Otherwise simulate minimal behavior
      const siftFn = (api as any).sift;
      if (typeof siftFn === "function") {
        const res = await siftFn(originalImage, { 
          contrastThreshold, 
          nfeatures,
        });
        setOutputImage(res.image ?? null);
        setKeypointsCount(res.featureCount ?? res.keypointsCount ?? null);
        setComputationTime(res.computationTime ?? null);
      } else {
        // No backend yet — keep UI wired and simulate a quick response so UI can be tested
        await new Promise((r) => setTimeout(r, 400));
        setOutputImage(originalImage);
        setKeypointsCount(0);
        setComputationTime(0);
      }
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : "An error occurred");
    } finally {
      setIsProcessing(false);
    }
  };

  return (
    <div className="flex flex-col lg:flex-row gap-6 h-full">
      {/* Images */}
      <div className="flex-1 grid grid-cols-2 gap-4 auto-rows-max">
        <ImageBox title="Input Image" image={originalImage} onUpload={handleUpload} />
        <ImageBox title="SIFT Keypoints" image={outputImage} />
      </div>

      {/* Controls */}
      <div className="w-full lg:w-72 shrink-0 space-y-4">
        {error && <p className="text-xs text-red-400">{error}</p>}
        {isProcessing && <p className="text-xs text-primary animate-pulse">Processing…</p>}

        <ParameterPanel
          contrastThreshold={contrastThreshold}
          setContrastThreshold={setContrastThreshold}
          nfeatures={nfeatures}
          setNfeatures={setNfeatures}
          runSIFT={runSIFT}
          originalImage={originalImage}
          isProcessing={isProcessing}
        />

        <StatisticsPanel computationTime={computationTime} keypointsCount={keypointsCount} />
      </div>
    </div>
  );
}
