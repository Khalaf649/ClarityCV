"use client";

import { useState, useCallback, useEffect, useRef } from "react";
import { ImageBox } from "@/components/ImageBox";
import { Slider } from "@/components/ui/Slider";
import { Select } from "@/components/ui/Select";
import { ControlPanel } from "@/components/ControlPanel";
import { useImageContext } from "@/contexts/ImageContext";
import { api } from "@/lib/api";

const filterTypeOptions = [
  { value: "low_pass",  label: "Low Pass" },
  { value: "high_pass", label: "High Pass" },
];

export function FrequencyFilterTab() {
  const { originalImage, setImageFromFile } = useImageContext();

  const [spectrum, setSpectrum]       = useState<string | null>(null);
  const [filtered, setFiltered]       = useState<string | null>(null);
  const [filterType, setFilterType]   = useState<"low_pass" | "high_pass">("low_pass");
  const [cutoff, setCutoff]           = useState(30);
  const [loading, setLoading]         = useState(false);
  const [error, setError]             = useState<string | null>(null);

  const debounceRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  const handleUpload = useCallback(
    async (file: File) => { await setImageFromFile(file); },
    [setImageFromFile],
  );

  useEffect(() => {
    setSpectrum(null);
    setFiltered(null);
  }, [originalImage]);

  // debounce calls whenever params change
  useEffect(() => {
    if (!originalImage) return;
    if (debounceRef.current) clearTimeout(debounceRef.current);
    debounceRef.current = setTimeout(async () => {
      setLoading(true); setError(null);
      try {
        const res = await api.frequency(originalImage, filterType, cutoff);
        setFiltered(res.image);
        setSpectrum(res.spectrum);
      } catch (e: unknown) {
        setError(e instanceof Error ? e.message : "Error");
      } finally {
        setLoading(false);
      }
    }, 300);
    return () => { if (debounceRef.current) clearTimeout(debounceRef.current); };
  }, [originalImage, filterType, cutoff]);

  return (
    <div className="flex flex-col lg:flex-row gap-6 h-full">
      <div className="flex-1 grid grid-cols-1 md:grid-cols-3 gap-4">
        <ImageBox title="Original Image"  image={originalImage} onUpload={handleUpload} />
        <ImageBox title="FFT Spectrum"    image={spectrum} />
        <ImageBox title="Filtered Result" image={filtered} />
      </div>

      <div className="w-full lg:w-72 shrink-0">
        {error   && <p className="text-xs text-red-400 mb-2">{error}</p>}
        {loading && <p className="text-xs text-primary animate-pulse mb-2">Processing…</p>}
        <ControlPanel title="Frequency Filter">
          <div className="space-y-3">
            <div>
              <label className="text-xs text-muted-foreground">Filter Type</label>
              <Select
                value={filterType}
                onValueChange={(v) => setFilterType(v as "low_pass" | "high_pass")}
                options={filterTypeOptions}
                className="mt-1"
              />
            </div>
            <div>
              <label className="text-xs text-muted-foreground">Cutoff: {cutoff}</label>
              <Slider value={[cutoff]} onValueChange={([v]) => setCutoff(v)} min={1} max={200} step={1} className="mt-2" />
            </div>
          </div>
        </ControlPanel>
      </div>
    </div>
  );
}
