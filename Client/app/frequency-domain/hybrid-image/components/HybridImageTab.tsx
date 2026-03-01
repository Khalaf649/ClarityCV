"use client";

import { useState, useCallback } from "react";
import { ImageBox } from "@/components/ImageBox";
import { Slider } from "@/components/ui/Slider";
import { Select } from "@/components/ui/Select";
import { Button } from "@/components/ui/Button";
import { ControlPanel } from "@/components/ControlPanel";
import {
  loadImageToCanvas,
  fft2d,
  fftToImage,
  createHybridImage,
  type FFTResult,
} from "@/lib/imageProcessing";

const passTypeOptions = [
  { value: "low", label: "Low Pass (Inner)" },
  { value: "high", label: "High Pass (Outer)" },
];

export function HybridImageTab() {
  const [img1, setImg1] = useState<ImageData | null>(null);
  const [img2, setImg2] = useState<ImageData | null>(null);
  const [fft1Img, setFft1Img] = useState<ImageData | null>(null);
  const [fft2Img, setFft2Img] = useState<ImageData | null>(null);
  const [fft1Data, setFft1Data] = useState<FFTResult | null>(null);
  const [fft2Data, setFft2Data] = useState<FFTResult | null>(null);
  const [output, setOutput] = useState<ImageData | null>(null);

  const [mode1, setMode1] = useState<"low" | "high">("low");
  const [mode2, setMode2] = useState<"low" | "high">("high");
  const [radius1, setRadius1] = useState(30);
  const [radius2, setRadius2] = useState(30);
  const [processing, setProcessing] = useState(false);

  const handleUpload1 = useCallback(async (file: File) => {
    const url = URL.createObjectURL(file);
    const { imageData } = await loadImageToCanvas(url);
    URL.revokeObjectURL(url);
    setImg1(imageData);
    setProcessing(true);
    setTimeout(() => {
      const fft = fft2d(imageData);
      setFft1Data(fft);
      setFft1Img(fftToImage(fft));
      setProcessing(false);
    }, 50);
  }, []);

  const handleUpload2 = useCallback(async (file: File) => {
    const url = URL.createObjectURL(file);
    const { imageData } = await loadImageToCanvas(url);
    URL.revokeObjectURL(url);
    setImg2(imageData);
    setProcessing(true);
    setTimeout(() => {
      const fft = fft2d(imageData);
      setFft2Data(fft);
      setFft2Img(fftToImage(fft));
      setProcessing(false);
    }, 50);
  }, []);

  const mix = useCallback(() => {
    if (!fft1Data || !fft2Data) return;
    setProcessing(true);
    setTimeout(() => {
      const result = createHybridImage(
        fft1Data,
        fft2Data,
        mode1,
        mode2,
        radius1,
        radius2,
      );
      setOutput(result);
      setProcessing(false);
    }, 50);
  }, [fft1Data, fft2Data, mode1, mode2, radius1, radius2]);

  return (
    <div className="flex flex-col gap-6 h-full">
      {/* Image row */}
      <div className="grid grid-cols-2 md:grid-cols-5 gap-4">
        <ImageBox title="Image 1" imageData={img1} onUpload={handleUpload1} />
        <ImageBox title="FFT 1" imageData={fft1Img} />
        <ImageBox title="Image 2" imageData={img2} onUpload={handleUpload2} />
        <ImageBox title="FFT 2" imageData={fft2Img} />
        <ImageBox title="Hybrid Output" imageData={output} />
      </div>

      {/* Controls row */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
        <ControlPanel title="Image 1">
          <div className="space-y-3">
            <div>
              <label className="text-xs text-muted-foreground">Pass Type</label>
              <Select
                value={mode1}
                onValueChange={(v) => setMode1(v as "low" | "high")}
                options={passTypeOptions}
                className="mt-1"
              />
            </div>
            <div>
              <label className="text-xs text-muted-foreground">
                Radius: {radius1}
              </label>
              <Slider
                value={[radius1]}
                onValueChange={([v]) => setRadius1(v)}
                min={1}
                max={200}
                step={1}
                className="mt-2"
              />
            </div>
          </div>
        </ControlPanel>

        <ControlPanel title="Image 2">
          <div className="space-y-3">
            <div>
              <label className="text-xs text-muted-foreground">Pass Type</label>
              <Select
                value={mode2}
                onValueChange={(v) => setMode2(v as "low" | "high")}
                options={passTypeOptions}
                className="mt-1"
              />
            </div>
            <div>
              <label className="text-xs text-muted-foreground">
                Radius: {radius2}
              </label>
              <Slider
                value={[radius2]}
                onValueChange={([v]) => setRadius2(v)}
                min={1}
                max={200}
                step={1}
                className="mt-2"
              />
            </div>
          </div>
        </ControlPanel>

        <ControlPanel title="Output" className="flex flex-col justify-center">
          {processing && (
            <p className="text-xs text-primary animate-pulse">Processing…</p>
          )}
          <Button
            onClick={mix}
            disabled={!fft1Data || !fft2Data || processing}
            className="w-full"
            size="sm"
          >
            Mix Images
          </Button>
        </ControlPanel>
      </div>
    </div>
  );
}
