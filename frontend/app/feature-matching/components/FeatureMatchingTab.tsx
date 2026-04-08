"use client";

import { useState, useCallback, useEffect } from "react";
import { ImageBox } from "@/components/ImageBox";
import { useImageContext } from "@/contexts/ImageContext";
import { ParameterPanel } from "./ParameterPanel";
import { StatisticsPanel } from "./StatisticsPanel";
import { fileToBase64 } from "@/lib/imageProcessing";
import { api } from "@/lib/api";

export function FeatureMatchingTab() {
  const { originalImage, setImageFromFile } = useImageContext();

  const [img2, setImg2] = useState<string | null>(null);
  const [outputImage, setOutputImage] = useState<string | null>(null);
  const [matchesCount, setMatchesCount] = useState<number | null>(null);
  const [selectedMethod, setSelectedMethod] = useState<string>("SSD");
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    setOutputImage(null);
    setMatchesCount(null);
  }, [originalImage, img2]);

  const handleUpload2 = useCallback(async (file: File) => {
    setImg2(await fileToBase64(file));
    setOutputImage(null);
  }, []);

  const handleUpload1 = useCallback(async (file: File) => {
    await setImageFromFile(file);
    setOutputImage(null);
  }, [setImageFromFile]);

  const runMatching = async () => {
    if (!originalImage || !img2) {
      setError("Please provide both input images.");
      return;
    }

    setLoading(true); setError(null);
    try {
      const fn = (api as any).featureMatching;
      if (typeof fn === "function") {
        const res = await fn(originalImage, img2, selectedMethod);
        setOutputImage(res.image ?? null);
        setMatchesCount(res.matchesCount ?? res.matchCount ?? null);
      } else {
        // No backend endpoint — keep UI wired and provide a simple passthrough
        await new Promise((r) => setTimeout(r, 300));
        setOutputImage(originalImage);
        setMatchesCount(0);
      }
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : "An error occurred");
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="flex flex-col lg:flex-row gap-6 h-full">
      <div className="flex-1 grid grid-cols-2 md:grid-cols-3 gap-4 auto-rows-max">
        <ImageBox title="Image 1" image={originalImage} onUpload={handleUpload1} />
        <ImageBox title="Image 2" image={img2} onUpload={handleUpload2} />
        <ImageBox title="Matches Output" image={outputImage} />
      </div>

      <div className="w-full lg:w-72 shrink-0 space-y-4">
        {error && <p className="text-xs text-red-400">{error}</p>}
        {loading && <p className="text-xs text-primary animate-pulse">Processing…</p>}

        <ParameterPanel
          selectedMethod={selectedMethod}
          setSelectedMethod={setSelectedMethod}
          runMatching={runMatching}
          originalImage={originalImage}
          img2={img2}
          loading={loading}
        />

        <StatisticsPanel matchesCount={matchesCount} selectedMethod={selectedMethod} />
      </div>
    </div>
  );
}
