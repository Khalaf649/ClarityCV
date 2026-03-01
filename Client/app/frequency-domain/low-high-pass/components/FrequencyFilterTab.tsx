"use client";

import { useState, useCallback, useEffect, useRef } from "react";
import { ImageBox } from "@/components/ImageBox";
import { Slider } from "@/components/ui/Slider";
import { Select } from "@/components/ui/Select";
import { ControlPanel } from "@/components/ControlPanel";
import { useImageContext } from "@/contexts/ImageContext";
import {
  fft2d,
  fftToImage,
  applyFrequencyFilter,
  ifft2d,
  type FFTResult,
} from "@/lib/imageProcessing";

const filterTypeOptions = [
  { value: "low", label: "Low Pass (Inner)" },
  { value: "high", label: "High Pass (Outer)" },
];

export function FrequencyFilterTab() {
  const { originalImage, setImageFromFile } = useImageContext();

  const [fftImage, setFftImage] = useState<ImageData | null>(null);
  const [filteredImage, setFilteredImage] = useState<ImageData | null>(null);
  const [fftData, setFftData] = useState<FFTResult | null>(null);

  const [filterType, setFilterType] = useState<"low" | "high">("low");
  const [radius, setRadius] = useState(30);
  const [processing, setProcessing] = useState(false);

  // Track whether initial FFT is being computed
  const computingFftRef = useRef(false);

  // Compute FFT when the shared image changes
  useEffect(() => {
    if (!originalImage) {
      setFftData(null);
      setFftImage(null);
      setFilteredImage(null);
      return;
    }

    computingFftRef.current = true;
    setProcessing(true);

    const timer = setTimeout(() => {
      const fft = fft2d(originalImage);
      setFftData(fft);
      // initial unfiltered spectrum
      setFftImage(fftToImage(fft));
      setFilteredImage(null);
      setProcessing(false);
      computingFftRef.current = false;
    }, 50);

    return () => {
      clearTimeout(timer);
      computingFftRef.current = false;
    };
  }, [originalImage]);

  // Real-time: recompute filtered FFT spectrum + IFFT result whenever
  // radius or filterType change (skip while initial FFT is in progress)
  useEffect(() => {
    if (!fftData || computingFftRef.current) return;

    setProcessing(true);

    const timer = setTimeout(() => {
      const filtered = applyFrequencyFilter(fftData, radius, filterType);
      setFftImage(fftToImage(filtered));
      setFilteredImage(ifft2d(filtered));
      setProcessing(false);
    }, 30);

    return () => clearTimeout(timer);
  }, [fftData, radius, filterType]);

  const handleUpload = useCallback(
    async (file: File) => {
      await setImageFromFile(file);
    },
    [setImageFromFile],
  );

  return (
    <div className="flex flex-col lg:flex-row gap-6 h-full">
      <div className="flex-1 grid grid-cols-1 md:grid-cols-3 gap-4">
        <ImageBox
          title="Original Image"
          imageData={originalImage}
          onUpload={handleUpload}
        />
        <ImageBox title="FFT Spectrum" imageData={fftImage} />
        <ImageBox title="Filtered Result" imageData={filteredImage} />
      </div>

      <div className="w-full lg:w-72 shrink-0">
        <ControlPanel title="Frequency Filter">
          {processing && (
            <p className="text-xs text-primary animate-pulse">Processing…</p>
          )}
          <div className="space-y-3">
            <div>
              <label className="text-xs text-muted-foreground">
                Filter Type
              </label>
              <Select
                value={filterType}
                onValueChange={(v) => setFilterType(v as "low" | "high")}
                options={filterTypeOptions}
                className="mt-1"
              />
            </div>
            <div>
              <label className="text-xs text-muted-foreground">
                Radius: {radius}
              </label>
              <Slider
                value={[radius]}
                onValueChange={([v]) => setRadius(v)}
                min={1}
                max={200}
                step={1}
                className="mt-2"
              />
            </div>
          </div>
        </ControlPanel>
      </div>
    </div>
  );
}
