"use client";

import { useState, useCallback } from "react";
import { ImageBox } from "@/components/ImageBox";
import { Slider } from "@/components/ui/Slider";
import { Button } from "@/components/ui/Button";
import { ControlPanel } from "@/components/ControlPanel";
import { fileToBase64 } from "@/lib/imageProcessing";
import { api } from "@/lib/api";

export function HybridImageTab() {
  const [img1, setImg1] = useState<string | null>(null);
  const [img2, setImg2] = useState<string | null>(null);

  const [lowFreq,  setLowFreq]  = useState<string | null>(null);
  const [highFreq, setHighFreq] = useState<string | null>(null);
  const [hybrid,   setHybrid]   = useState<string | null>(null);

  const [lowCutoff,  setLowCutoff]  = useState(30);
  const [highCutoff, setHighCutoff] = useState(30);
  const [alpha,      setAlpha]      = useState(0.5);
  const [loading,    setLoading]    = useState(false);
  const [error,      setError]      = useState<string | null>(null);

  const handleUpload1 = useCallback(async (file: File) => {
    setImg1(await fileToBase64(file));
    setHybrid(null);
  }, []);

  const handleUpload2 = useCallback(async (file: File) => {
    setImg2(await fileToBase64(file));
    setHybrid(null);
  }, []);

  const mix = async () => {
    if (!img1 || !img2) return;
    setLoading(true); setError(null);
    try {
      const res = await api.hybrid(img1, img2, lowCutoff, highCutoff, alpha);
      setLowFreq(res.low_freq_image);
      setHighFreq(res.high_freq_image);
      setHybrid(res.hybrid_image);
    } catch (e: unknown) {
      setError(e instanceof Error ? e.message : "Error");
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="flex flex-col gap-6 h-full">
      <div className="grid grid-cols-2 md:grid-cols-5 gap-4">
        <ImageBox title="Image 1 (Low Pass)"  image={img1}     onUpload={handleUpload1} />
        <ImageBox title="Low Freq Result"     image={lowFreq} />
        <ImageBox title="Image 2 (High Pass)" image={img2}     onUpload={handleUpload2} />
        <ImageBox title="High Freq Result"    image={highFreq} />
        <ImageBox title="Hybrid Output"       image={hybrid} />
      </div>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
        <ControlPanel title="Image 1 — Low Pass Cutoff">
          <div>
            <label className="text-xs text-muted-foreground">Cutoff: {lowCutoff}</label>
            <Slider value={[lowCutoff]} onValueChange={([v]) => setLowCutoff(v)} min={1} max={200} step={1} className="mt-2" />
          </div>
        </ControlPanel>

        <ControlPanel title="Image 2 — High Pass Cutoff">
          <div>
            <label className="text-xs text-muted-foreground">Cutoff: {highCutoff}</label>
            <Slider value={[highCutoff]} onValueChange={([v]) => setHighCutoff(v)} min={1} max={200} step={1} className="mt-2" />
          </div>
        </ControlPanel>

        <ControlPanel title="Blend">
          <div className="space-y-3">
            <div>
              <label className="text-xs text-muted-foreground">Alpha: {alpha.toFixed(2)}</label>
              <Slider value={[alpha * 100]} onValueChange={([v]) => setAlpha(v / 100)} min={0} max={100} step={1} className="mt-2" />
            </div>
            {error   && <p className="text-xs text-red-400">{error}</p>}
            {loading && <p className="text-xs text-primary animate-pulse">Processing…</p>}
            <Button onClick={mix} disabled={!img1 || !img2 || loading} className="w-full" size="sm">
              Mix Images
            </Button>
          </div>
        </ControlPanel>
      </div>
    </div>
  );
}
