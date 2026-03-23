"use client";

import { useState, useCallback, useEffect } from "react";
import { ImageBox } from "@/components/ImageBox";
import { useImageContext } from "@/contexts/ImageContext";
import { api } from "@/lib/api";
import { ParametersPanel } from "./ParametersPanel";
import { ActionButtons } from "./ActionButtons";
import { StatisticsPanel } from "./StatisticsPanel";

const DEFAULT_ALPHA = 1.0;
const DEFAULT_BETA = 1.0;
const DEFAULT_GAMMA = 1.0;
const DEFAULT_ITERATIONS = 100;

export function ActiveContourTab() {
  const { originalImage, setImageFromFile } = useImageContext();

  // Image state
  const [outputImage, setOutputImage] = useState<string | null>(null);
  
  // Points state
  const [activePoints, setActivePoints] = useState<Array<{ x: number; y: number }>>([]);

  // Statistics state
  const [perimeter, setPerimeter] = useState<number | null>(null);
  const [area, setArea] = useState<number | null>(null);

  // Parameter sliders
  const [alpha, setAlpha] = useState(DEFAULT_ALPHA);
  const [beta, setBeta] = useState(DEFAULT_BETA);
  const [gamma, setGamma] = useState(DEFAULT_GAMMA);
  const [iterations, setIterations] = useState(DEFAULT_ITERATIONS);

  // UI state
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

    // setPerimeter(null);
    // setArea(null);
  // Clear output when image changes
  useEffect(() => {
    setOutputImage(null);
    setActivePoints([]);
  }, [originalImage]);

  const handleUpload = useCallback(
    async (file: File) => {
      await setImageFromFile(file);
    },
    [setImageFromFile],
  );

  const handleImageClick = useCallback(
    (coords: { x: number; y: number }) => {
      setActivePoints((prev) => [...prev, coords]);
    },
    [],
  );

  const handleApply = async () => {
    if (!originalImage || activePoints.length === 0) {
      setError("Please upload an image and place at least one control point");
      return;
    }

    setLoading(true);
    setError(null);
    try {
      const res = await api.activeContour(
        originalImage,
        alpha,
        beta,
        gamma,
        iterations,
        activePoints,
      
      );
      setPerimeter(res.perimeter);
      setArea(res.area);
      setOutputImage(res.image);
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : "Error processing active contour");
    } finally {
      setLoading(false);
    }
  };

  const handleReset = () => {
    setActivePoints([]);
    setOutputImage(null);
    setAlpha(DEFAULT_ALPHA);
    setBeta(DEFAULT_BETA);
    setGamma(DEFAULT_GAMMA);
    setIterations(DEFAULT_ITERATIONS);
    setError(null);
    setArea(null);
    setPerimeter(null);
  };

  return (
    <div className="flex flex-col lg:flex-row gap-6 h-full">
      {/* Images */}
      <div className="flex-1 grid grid-cols-2 gap-4">
        <ImageBox
          title="Input"
          image={originalImage}
          onUpload={handleUpload}
          activePoints={activePoints}
          onImageClick={handleImageClick}
        />
        <ImageBox title="Output" image={outputImage} />
      </div>

      {/* Controls */}
      <div className="w-full lg:w-72 shrink-0 space-y-4">
        {error && <p className="text-xs text-red-400">{error}</p>}
        {loading && <p className="text-xs text-primary animate-pulse">Processing…</p>}
        {activePoints.length > 0 && (
          <p className="text-xs text-muted-foreground">
            {activePoints.length} point{activePoints.length !== 1 ? "s" : ""} placed
          </p>
        )}

        <ParametersPanel
          alpha={alpha}
          beta={beta}
          gamma={gamma}
          iterations={iterations}
          onAlphaChange={setAlpha}
          onBetaChange={setBeta}
          onGammaChange={setGamma}
          onIterationsChange={setIterations}
        />

        <StatisticsPanel perimeter={perimeter} area={area} />

        <ActionButtons
          onApply={handleApply}
          onReset={handleReset}
          applyDisabled={!originalImage || activePoints.length === 0}
          loading={loading}
        />
      </div>
    </div>
  );
}
