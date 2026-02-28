"use client";

import { useState, useCallback } from "react";
import { ImageBox } from "./ImageBox";
import { Slider } from "./ui/Slider";
import { Select } from "./ui/Select";
import { Button } from "./ui/Button";
import { ControlPanel } from "./ControlPanel";
import {
  loadImageToCanvas,
  fft2d,
  fftToImage,
  applyFrequencyFilter,
  ifft2d,
  type FFTResult,
} from "@/lib/imageProcessing";

const filterTypeOptions = [
  { value: "low", label: "Low Pass" },
  { value: "high", label: "High Pass" },
];

export function FrequencyFilterTab() {
  const [original, setOriginal] = useState<ImageData | null>(null);
  const [fftImage, setFftImage] = useState<ImageData | null>(null);
  const [filteredImage, setFilteredImage] = useState<ImageData | null>(null);
  const [fftData, setFftData] = useState<FFTResult | null>(null);

  const [filterType, setFilterType] = useState<"low" | "high">("low");
  const [radius, setRadius] = useState(30);
  const [processing, setProcessing] = useState(false);

  const handleUpload = useCallback(async (file: File) => {
    const url = URL.createObjectURL(file);
    const { imageData } = await loadImageToCanvas(url);
    URL.revokeObjectURL(url);
    setOriginal(imageData);
    setProcessing(true);
    setTimeout(() => {
      const fft = fft2d(imageData);
      setFftData(fft);
      setFftImage(fftToImage(fft));
      setFilteredImage(null);
      setProcessing(false);
    }, 50);
  }, []);

  const applyFilter = useCallback(() => {
    if (!fftData) return;
    setProcessing(true);
    setTimeout(() => {
      const filtered = applyFrequencyFilter(fftData, radius, filterType);
      setFilteredImage(ifft2d(filtered));
      setProcessing(false);
    }, 50);
  }, [fftData, radius, filterType]);

  return (
    <div className="flex flex-col lg:flex-row gap-6 h-full">
      <div className="flex-1 grid grid-cols-1 md:grid-cols-3 gap-4">
        <ImageBox
          title="Original Image"
          imageData={original}
          onUpload={handleUpload}
        />
        <ImageBox title="FFT Spectrum" imageData={fftImage} />
        <ImageBox title="Filtered Result" imageData={filteredImage} />
      </div>

      <div className="w-full lg:w-72 shrink-0">
        <ControlPanel title="Frequency Filter">
          {processing && (
            <p className="text-xs text-primary animate-pulse">Processing...</p>
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
            <Button
              onClick={applyFilter}
              disabled={!fftData || processing}
              className="w-full"
              size="sm"
            >
              Apply Filter
            </Button>
          </div>
        </ControlPanel>
      </div>
    </div>
  );
}
