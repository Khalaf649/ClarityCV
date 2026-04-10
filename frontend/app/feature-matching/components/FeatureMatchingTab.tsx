"use client";

import { useState, useCallback, useEffect } from "react";
import { ImageBox }          from "@/components/ImageBox";
import { useImageContext }   from "@/contexts/ImageContext";
import { ParameterPanel }    from "./ParameterPanel";
import { StatisticsPanel }   from "./StatisticsPanel";
import { fileToBase64 }      from "@/lib/imageProcessing";
import { api }               from "@/lib/api";

export function FeatureMatchingTab() {
  const { originalImage, setImageFromFile } = useImageContext();

  const [img2,           setImg2]           = useState<string | null>(null);
  const [outputImage,    setOutputImage]    = useState<string | null>(null);
  const [selectedMethod, setSelectedMethod] = useState<string>("SSD");
  const [maxMatches,     setMaxMatches]     = useState<number>(50);
  const [ratioThreshold, setRatioThreshold] = useState<number>(0.75);
  const [loading,        setLoading]        = useState(false);
  const [error,          setError]          = useState<string | null>(null);

  // Statistics state
  const [matchesCount,   setMatchesCount]   = useState<number | null>(null);
  const [computationTime,setComputationTime]= useState<number | null>(null);
  const [siftTimeImg1,   setSiftTimeImg1]   = useState<number | null>(null);
  const [siftTimeImg2,   setSiftTimeImg2]   = useState<number | null>(null);
  const [keypointsImg1,  setKeypointsImg1]  = useState<number | null>(null);
  const [keypointsImg2,  setKeypointsImg2]  = useState<number | null>(null);

  // Reset results when either input image changes
  useEffect(() => {
    setOutputImage(null);
    setMatchesCount(null);
    setComputationTime(null);
    setSiftTimeImg1(null);
    setSiftTimeImg2(null);
    setKeypointsImg1(null);
    setKeypointsImg2(null);
  }, [originalImage, img2]);

  const handleUpload2 = useCallback(async (file: File) => {
    setImg2(await fileToBase64(file));
  }, []);

  const handleUpload1 = useCallback(async (file: File) => {
    await setImageFromFile(file);
  }, [setImageFromFile]);

  const runMatching = async () => {
    if (!originalImage || !img2) {
      setError("Please provide both input images.");
      return;
    }

    setLoading(true);
    setError(null);

    try {
      const res = await api.featureMatching(
        originalImage,
        img2,
        selectedMethod as "SSD" | "NCC",
        maxMatches,
        ratioThreshold,
      );

      setOutputImage(res.image      ?? null);
      setMatchesCount(res.matchesCount ?? null);
      setComputationTime(res.computationTime ?? null);
      setSiftTimeImg1(res.siftTimeImg1 ?? null);
      setSiftTimeImg2(res.siftTimeImg2 ?? null);
      setKeypointsImg1(res.keypointsImg1 ?? null);
      setKeypointsImg2(res.keypointsImg2 ?? null);
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : "An error occurred");
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="flex flex-col lg:flex-row gap-6 h-full">
      {/* Image panels */}
      <div className="flex-1 grid grid-cols-2 md:grid-cols-3 gap-4 auto-rows-max">
        <ImageBox title="Image 1"       image={originalImage} onUpload={handleUpload1} />
        <ImageBox title="Image 2"       image={img2}          onUpload={handleUpload2} />
        <ImageBox title="Matches Output" image={outputImage} />
      </div>

      {/* Side panel */}
      <div className="w-full lg:w-72 shrink-0 space-y-4">
        {error   && <p className="text-xs text-red-400">{error}</p>}
        {loading && <p className="text-xs text-primary animate-pulse">Processing…</p>}

        <ParameterPanel
          selectedMethod={selectedMethod}
          setSelectedMethod={setSelectedMethod}
          maxMatches={maxMatches}
          setMaxMatches={setMaxMatches}
          ratioThreshold={ratioThreshold}
          setRatioThreshold={setRatioThreshold}
          runMatching={runMatching}
          originalImage={originalImage}
          img2={img2}
          loading={loading}
        />

        <StatisticsPanel
          matchesCount={matchesCount}
          selectedMethod={selectedMethod}
          computationTime={computationTime}
          siftTimeImg1={siftTimeImg1}
          siftTimeImg2={siftTimeImg2}
          keypointsImg1={keypointsImg1}
          keypointsImg2={keypointsImg2}
        />
      </div>
    </div>
  );
}
