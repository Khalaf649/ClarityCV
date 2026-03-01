"use client";

import { useState, useCallback, useEffect } from "react";
import { ImageBox } from "@/components/ImageBox";
import { Button } from "@/components/ui/Button";
import { ControlPanel } from "@/components/ControlPanel";
import { useImageContext } from "@/contexts/ImageContext";
import { api } from "@/lib/api";
import { toDataUrl } from "@/lib/imageProcessing";

export function HistogramTab() {
  const { originalImage, setImageFromFile } = useImageContext();

  const [currentImage, setCurrentImage] = useState<string | null>(null);
  const [histogramPlot, setHistogramPlot] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    setCurrentImage(originalImage);
    setHistogramPlot(null);
  }, [originalImage]);

  // fetch histogram whenever currentImage changes
  useEffect(() => {
    if (!currentImage) { setHistogramPlot(null); return; }
    (async () => {
      try {
        const res = await api.histogram(currentImage);
        setHistogramPlot(res.plot);
      } catch { /* silent */ }
    })();
  }, [currentImage]);

  const handleUpload = useCallback(
    async (file: File) => { await setImageFromFile(file); },
    [setImageFromFile],
  );

  const run = async (fn: () => Promise<{ image: string }>) => {
    if (!currentImage) return;
    setLoading(true); setError(null);
    try {
      const res = await fn();
      setCurrentImage(res.image);
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : "Error");
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="flex flex-col lg:flex-row gap-6 h-full">
      <div className="flex-1 grid grid-cols-1 md:grid-cols-2 gap-4">
        <ImageBox title="Image" image={currentImage} onUpload={handleUpload} />

        <div className="flex flex-col gap-2">
          <span className="text-xs font-mono text-muted-foreground uppercase tracking-wider">
            Histogram
          </span>
          <div className="image-box glow-border aspect-[4/3]">
            {histogramPlot
              ? <img src={toDataUrl(histogramPlot)} alt="Histogram" className="w-full h-full object-contain" />
              : <span className="text-xs text-muted-foreground">No image</span>
            }
          </div>
        </div>
      </div>

      <div className="w-full lg:w-72 shrink-0">
        {error   && <p className="text-xs text-red-400 mb-2">{error}</p>}
        {loading && <p className="text-xs text-primary animate-pulse mb-2">Processing…</p>}
        <ControlPanel title="Operations">
          <div className="space-y-2">
            <Button onClick={() => run(() => api.normalize(currentImage!))} disabled={!currentImage || loading} className="w-full" size="sm" variant="secondary">
              Normalize
            </Button>
            <Button onClick={() => run(() => api.equalize(currentImage!))} disabled={!currentImage || loading} className="w-full" size="sm" variant="secondary">
              Equalize
            </Button>
            <Button onClick={() => { setCurrentImage(originalImage); setHistogramPlot(null); }} disabled={!originalImage} className="w-full" size="sm" variant="outline">
              Reset to Original
            </Button>
          </div>
        </ControlPanel>
      </div>
    </div>
  );
}
