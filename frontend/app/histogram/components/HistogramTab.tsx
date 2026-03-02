"use client";

import { useState, useCallback, useEffect } from "react";
import { ImageBox } from "@/components/ImageBox";
import { Button } from "@/components/ui/Button";
import { ControlPanel } from "@/components/ControlPanel";
import { useImageContext } from "@/contexts/ImageContext";
import { api, HistogramChannelWithStats } from "@/lib/api";
import { toDataUrl } from "@/lib/imageProcessing";

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

type PlotMode = "histogram" | "histogram_curve";

// ---------------------------------------------------------------------------
// Component
// ---------------------------------------------------------------------------

export function HistogramTab() {
  const { originalImage, setImageFromFile } = useImageContext();

  const [currentImage, setCurrentImage]       = useState<string | null>(null);
  const [histogramPlot, setHistogramPlot]       = useState<string | null>(null);
  const [curvePlot, setCurvePlot]               = useState<string | null>(null);
  const [channelStats, setChannelStats]         = useState<HistogramChannelWithStats[]>([]);
  const [plotMode, setPlotMode]                 = useState<PlotMode>("histogram");
  const [loading, setLoading]                   = useState(false);
  const [plotLoading, setPlotLoading]           = useState(false);
  const [error, setError]                       = useState<string | null>(null);

  // sync with global uploaded image
  useEffect(() => {
    setCurrentImage(originalImage);
    setHistogramPlot(null);
    setCurvePlot(null);
    setChannelStats([]);
  }, [originalImage]);

  // fetch both plots whenever currentImage changes
  useEffect(() => {
    if (!currentImage) {
      setHistogramPlot(null);
      setCurvePlot(null);
      setChannelStats([]);
      return;
    }

    setPlotLoading(true);

    // Always fetch the curve endpoint — it returns both plots + stats
    api.histogramCurve(currentImage)
      .then((res) => {
        setHistogramPlot(res.plot);
        setCurvePlot(res.plot_curve);
        setChannelStats(res.channels);
      })
      .catch(() => {
        // Fallback to plain histogram if curve endpoint not available
        api.histogram(currentImage)
          .then((res) => {
            setHistogramPlot(res.plot);
            setCurvePlot(null);
          })
          .catch(() => {/* silent */});
      })
      .finally(() => setPlotLoading(false));
  }, [currentImage]);

  const handleUpload = useCallback(
    async (file: File) => { await setImageFromFile(file); },
    [setImageFromFile],
  );

  const run = async (fn: () => Promise<{ image: string }>) => {
    if (!currentImage) return;
    setLoading(true);
    setError(null);
    try {
      const res = await fn();
      setCurrentImage(res.image);
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : "Error");
    } finally {
      setLoading(false);
    }
  };

  // Which plot image to show based on selected tab
  const activePlot =
    plotMode === "histogram_curve" && curvePlot ? curvePlot : histogramPlot;

  const plotTitle =
    plotMode === "histogram" ? "Histogram" : "Histogram & Distribution Curve";

  return (
    <div className="flex flex-col lg:flex-row gap-6 h-full">
      {/* ------------------------------------------------------------------ */}
      {/* Left: images                                                         */}
      {/* ------------------------------------------------------------------ */}
      <div className="flex-1 grid grid-cols-1 md:grid-cols-2 gap-4">
        {/* Uploaded / processed image */}
        <ImageBox title="Image" image={currentImage} onUpload={handleUpload} />

        {/* Histogram / Curve plot */}
        <div className="flex flex-col gap-2">
          {/* Plot mode toggle tabs */}
          <div className="flex items-center gap-1">
            <button
              onClick={() => setPlotMode("histogram")}
              className={`text-xs font-mono px-2 py-1 rounded transition-colors ${
                plotMode === "histogram"
                  ? "bg-primary text-primary-foreground"
                  : "text-muted-foreground hover:text-foreground"
              }`}
            >
              Histogram
            </button>
            <button
              onClick={() => setPlotMode("histogram_curve")}
              disabled={!curvePlot}
              className={`text-xs font-mono px-2 py-1 rounded transition-colors disabled:opacity-40 ${
                plotMode === "histogram_curve"
                  ? "bg-primary text-primary-foreground"
                  : "text-muted-foreground hover:text-foreground"
              }`}
            >
              Histogram &amp; Distribution Curve
            </button>
          </div>

          {/* Plot box */}
          <div className="image-box glow-border aspect-[4/3] relative">
            {plotLoading && (
              <span className="text-xs text-primary animate-pulse">
                Computing…
              </span>
            )}
            {!plotLoading && activePlot ? (
              <img
                src={toDataUrl(activePlot)}
                alt={plotTitle}
                className="w-full h-full object-contain"
              />
            ) : (
              !plotLoading && (
                <span className="text-xs text-muted-foreground">No image</span>
              )
            )}
          </div>

          {/* Per-channel stats (only in curve mode) */}
          {plotMode === "histogram_curve" && channelStats.length > 0 && (
            <div className="flex flex-wrap gap-3 mt-1">
              {channelStats.map((ch) => (
                <div
                  key={ch.label}
                  className="flex flex-col gap-0.5 bg-muted/40 rounded px-2 py-1"
                >
                  <span className="text-xs font-mono font-semibold text-foreground">
                    {ch.label}
                  </span>
                  <span className="text-xs text-muted-foreground">
                    μ&nbsp;=&nbsp;{ch.mean.toFixed(1)}
                  </span>
                  <span className="text-xs text-muted-foreground">
                    σ&nbsp;=&nbsp;{ch.stddev.toFixed(1)}
                  </span>
                </div>
              ))}
            </div>
          )}
        </div>
      </div>

      {/* ------------------------------------------------------------------ */}
      {/* Right: controls                                                      */}
      {/* ------------------------------------------------------------------ */}
      <div className="w-full lg:w-72 shrink-0">
        {error   && <p className="text-xs text-red-400 mb-2">{error}</p>}
        {loading && (
          <p className="text-xs text-primary animate-pulse mb-2">
            Processing…
          </p>
        )}

        <ControlPanel title="Operations">
          <div className="space-y-2">
            <Button
              onClick={() => run(() => api.normalize(currentImage!))}
              disabled={!currentImage || loading}
              className="w-full"
              size="sm"
              variant="secondary"
            >
              Normalize
            </Button>

            <Button
              onClick={() => run(() => api.equalize(currentImage!))}
              disabled={!currentImage || loading}
              className="w-full"
              size="sm"
              variant="secondary"
            >
              Equalize
            </Button>

            <Button
              onClick={() => {
                setCurrentImage(originalImage);
                setHistogramPlot(null);
                setCurvePlot(null);
                setChannelStats([]);
              }}
              disabled={!originalImage}
              className="w-full"
              size="sm"
              variant="outline"
            >
              Reset to Original
            </Button>
          </div>
        </ControlPanel>
      </div>
    </div>
  );
}