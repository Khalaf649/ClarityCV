"use client";

import { useState, useCallback } from "react";
import { ImageBox } from "./ImageBox";
import { Slider } from "./ui/Slider";
import { Select } from "./ui/Select";
import { Button } from "./ui/Button";
import { ControlPanel } from "./ControlPanel";
import {
  loadImageToCanvas,
  cloneImageData,
  addUniformNoise,
  addGaussianNoise,
  addSaltPepperNoise,
  averageFilter,
  gaussianFilter,
  medianFilter,
  sobelFilter,
  robertsFilter,
  prewittFilter,
  cannyEdgeDetector,
  equalizeImage,
} from "@/lib/imageProcessing";

const noiseOptions = [
  { value: "uniform", label: "Uniform" },
  { value: "gaussian", label: "Gaussian" },
  { value: "salt_pepper", label: "Salt & Pepper" },
];

const filterOptions = [
  { value: "average", label: "Average" },
  { value: "gaussian", label: "Gaussian" },
  { value: "median", label: "Median" },
  { value: "sobel", label: "Sobel" },
  { value: "roberts", label: "Roberts" },
  { value: "prewitt", label: "Prewitt" },
  { value: "canny", label: "Canny" },
  { value: "equalize", label: "Equalize" },
];

export function SpatialFilterTab() {
  const [original, setOriginal] = useState<ImageData | null>(null);
  const [noisy, setNoisy] = useState<ImageData | null>(null);
  const [filtered, setFiltered] = useState<ImageData | null>(null);

  const [noiseType, setNoiseType] = useState("gaussian");
  const [noisePercent, setNoisePercent] = useState(20);
  const [filterType, setFilterType] = useState("average");
  const [kernelSize, setKernelSize] = useState(3);

  const handleUpload = useCallback(async (file: File) => {
    const url = URL.createObjectURL(file);
    const { imageData } = await loadImageToCanvas(url);
    URL.revokeObjectURL(url);
    setOriginal(imageData);
    setNoisy(null);
    setFiltered(null);
  }, []);

  const applyNoise = useCallback(() => {
    if (!original) return;
    const src = cloneImageData(original);
    let result: ImageData;
    switch (noiseType) {
      case "uniform":
        result = addUniformNoise(src, noisePercent);
        break;
      case "salt_pepper":
        result = addSaltPepperNoise(src, noisePercent);
        break;
      default:
        result = addGaussianNoise(src, noisePercent);
    }
    setNoisy(result);
    setFiltered(null);
  }, [original, noiseType, noisePercent]);

  const applyFilter = useCallback(() => {
    const src = noisy || original;
    if (!src) return;
    let result: ImageData;
    const k = kernelSize % 2 === 0 ? kernelSize + 1 : kernelSize;
    switch (filterType) {
      case "gaussian":
        result = gaussianFilter(src, k);
        break;
      case "median":
        result = medianFilter(src, k);
        break;
      case "sobel":
        result = sobelFilter(src);
        break;
      case "roberts":
        result = robertsFilter(src);
        break;
      case "prewitt":
        result = prewittFilter(src);
        break;
      case "canny":
        result = cannyEdgeDetector(src, k);
        break;
      case "equalize":
        result = equalizeImage(src);
        break;
      default:
        result = averageFilter(src, k);
    }
    setFiltered(result);
  }, [noisy, original, filterType, kernelSize]);

  const isEdgeFilter = ["sobel", "roberts", "prewitt"].includes(filterType);

  return (
    <div className="flex flex-col lg:flex-row gap-6 h-full">
      {/* Image grid */}
      <div className="flex-1 grid grid-cols-1 md:grid-cols-3 gap-4">
        <ImageBox
          title="Original Image"
          imageData={original}
          onUpload={handleUpload}
        />
        <ImageBox title="After Noise" imageData={noisy} />
        <ImageBox title="After Filter" imageData={filtered} />
      </div>

      {/* Controls */}
      <div className="w-full lg:w-72 shrink-0 space-y-6">
        <ControlPanel title="Noise">
          <div className="space-y-3">
            <div>
              <label className="text-xs text-muted-foreground">Type</label>
              <Select
                value={noiseType}
                onValueChange={setNoiseType}
                options={noiseOptions}
                className="mt-1"
              />
            </div>
            <div>
              <label className="text-xs text-muted-foreground">
                Percentage: {noisePercent}%
              </label>
              <Slider
                value={[noisePercent]}
                onValueChange={([v]) => setNoisePercent(v)}
                min={1}
                max={100}
                step={1}
                className="mt-2"
              />
            </div>
            <Button
              onClick={applyNoise}
              disabled={!original}
              className="w-full"
              size="sm"
            >
              Apply Noise
            </Button>
          </div>
        </ControlPanel>

        <ControlPanel title="Filter">
          <div className="space-y-3">
            <div>
              <label className="text-xs text-muted-foreground">Type</label>
              <Select
                value={filterType}
                onValueChange={setFilterType}
                options={filterOptions}
                className="mt-1"
              />
            </div>
            {!isEdgeFilter && filterType !== "equalize" && (
              <div>
                <label className="text-xs text-muted-foreground">
                  Kernel Size:{" "}
                  {kernelSize % 2 === 0 ? kernelSize + 1 : kernelSize}
                </label>
                <Slider
                  value={[kernelSize]}
                  onValueChange={([v]) => setKernelSize(v)}
                  min={3}
                  max={15}
                  step={2}
                  className="mt-2"
                />
              </div>
            )}
            <Button
              onClick={applyFilter}
              disabled={!original}
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
