"use client";

import { useState, useCallback, useEffect } from "react";
import { ImageBox } from "@/components/ImageBox";
import { Slider } from "@/components/ui/Slider";
import { Select } from "@/components/ui/Select";
import { Button } from "@/components/ui/Button";
import { ControlPanel } from "@/components/ControlPanel";
import { useImageContext } from "@/contexts/ImageContext";
import { api } from "@/lib/api";

const shapeOptions = [
  { value: "line",    label: "Line" },
  { value: "circle",  label: "Circle" },
  { value: "ellipse", label: "Ellipse" },
];

export function HoughTransformTab() {
  const { originalImage, setImageFromFile } = useImageContext();

  const [outputImage, setOutputImage] = useState<string | null>(null);

  const [shapeType, setShapeType] = useState("line");
  const [votesThreshold, setVotesThreshold] = useState(100);
  
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    setOutputImage(null);
  }, [originalImage]);

  const handleUpload = useCallback(
    async (file: File) => { await setImageFromFile(file); },
    [setImageFromFile],
  );

  const applyHough = async () => {
    if (!originalImage) return;
    setLoading(true); setError(null);
    try {
      const res = await api.houghTransform(originalImage, shapeType as "line" | "circle" | "ellipse", votesThreshold);
      setOutputImage(res.image);
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : "Error");
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="flex flex-col lg:flex-row gap-6 h-full">
      {/* Images */}
      <div className="flex-1 grid grid-cols-2 gap-4 auto-rows-max">
        <ImageBox title="Input Image" image={originalImage} onUpload={handleUpload} />
        <ImageBox title="Hough Transform Output" image={outputImage} />
      </div>

      {/* Controls */}
      <div className="w-full lg:w-72 shrink-0 space-y-4">
        {error && <p className="text-xs text-red-400">{error}</p>}
        {loading && <p className="text-xs text-primary animate-pulse">Processing…</p>}

        <ControlPanel title="Hough Transform">
          <div className="space-y-3">
            <div>
              <label className="text-xs text-muted-foreground">Shape Type</label>
              <Select value={shapeType} onValueChange={setShapeType} options={shapeOptions} className="mt-1" />
            </div>
            <div>
              <label className="text-xs text-muted-foreground">Votes Threshold: {votesThreshold}</label>
              <Slider value={[votesThreshold]} onValueChange={([v]) => setVotesThreshold(v)} min={1} max={300} step={1} className="mt-2" />
            </div>
            <Button onClick={applyHough} disabled={!originalImage || loading} className="w-full" size="sm">
              Apply
            </Button>
          </div>
        </ControlPanel>
      </div>
    </div>
  );
}
