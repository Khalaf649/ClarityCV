"use client";
import { useCallback, useState } from "react";
import { useImageContext } from "@/contexts/ImageContext";
import { ImageBox } from "@/components/ImageBox";
import { ParameterPanel } from "../components/ParametersPanel";
import { ActionButtons } from "../components/ActiveButtons";
import { StatisticsPanel } from "../components/StatisticsPanel";
import { api } from "@/lib/api";
export default function FaceRecognitionTab() {
  const { originalImage, setImageFromFile } = useImageContext();
  const [outputImage, setOutputImage] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [selectedMethod, setSelectedMethod] = useState<string | null>(null);
  const [threshold, setThreshold] = useState<number>(5);
  const [computionTime, setComputationTime] = useState<number | null>(null);
  const [faceCount, setFaceCount] = useState<number | null>(null);
  const handleApply = useCallback(async () => {
    if (!originalImage) return;

    setLoading(true);
    setError(null);

    try {
      const response = await api.recognizeFaces(originalImage, {
        method: selectedMethod || "face_recognition",
        threshold,
      });
      setOutputImage(response.image);
      setComputationTime(response.computationTime);
      setFaceCount(response.facesDetected);
    } catch (err) {
      setError("Failed to recognize faces.");
    } finally {
      setLoading(false);
    }
  }, [originalImage, selectedMethod, threshold]);

  const handleUpload = useCallback(
    async (file: File) => {
      await setImageFromFile(file);
    },
    [setImageFromFile],
  );
  const handleReset = useCallback(() => {
    setOutputImage(null);
    setSelectedMethod("Face Recognition");
    setThreshold(5);
  }, []);

  return (
    <div className="flex  lg:flex-row gap-6 h-full">
      <div className="flex-1 grid grid-cols-2 gap-4">
        <ImageBox title="Input" image={originalImage} onUpload={handleUpload} />
        <ImageBox title="Output" image={outputImage} />
      </div>
      <div className="w-full lg:w-72 shrink-0 space-y-4">
        {error && <p className="text-xs text-red-400">{error}</p>}
        {loading && (
          <p className="text-xs text-primary animate-pulse">Processing…</p>
        )}
        <ParameterPanel
          selectedMethod={selectedMethod || "face_recognition"}
          onMethodChange={setSelectedMethod}
          selectedThreshold={threshold}
          onThresholdChange={setThreshold}
        />

        <ActionButtons
          onApply={handleApply}
          onReset={handleReset}
          applyDisabled={!originalImage}
          loading={loading}
        />

        <StatisticsPanel
          computationTime={computionTime}
          facesDetected={faceCount}
          method={selectedMethod}
        />
      </div>
    </div>
  );
}
