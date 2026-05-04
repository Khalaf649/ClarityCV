"use client";

import { useState } from "react";
import { useImageContext } from "@/contexts/ImageContext";
import { ImageBox } from "@/components/ImageBox";
import { ParameterTable } from "./components/ParameterTable";
import { ActionButtons } from "../components/ActionButtons";
import { api } from "@/lib/api";

export default function AdvancedPage() {
  const { originalImage } = useImageContext();
  const [selectedMethod, setSelectedMethod] = useState("optimal");
  const [outputImage, setOutputImage] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);

  const handleApply = async () => {
    if (!originalImage) return;

    setLoading(true);
    try {
      const data = await api.segmentationAdvanced(
        originalImage,
        selectedMethod as "optimal" | "otsu" | "spectral" | "local",
      );
      setOutputImage(data.result);
    } catch (error) {
      console.error("Error applying advanced segmentation:", error);
    } finally {
      setLoading(false);
    }
  };

  const handleReset = () => {
    setOutputImage(null);
    setSelectedMethod("optimal");
  };

  return (
    <main className="flex-1 p-6 overflow-auto">
      <div className="space-y-6">
        {/* Title */}
        <div>
          <h1 className="text-2xl font-bold text-foreground">
            Advanced Segmentation
          </h1>
          <p className="text-sm text-muted-foreground mt-1">
            Apply advanced segmentation techniques to your image
          </p>
        </div>

        {/* Main Content Grid */}
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          {/* Input Image */}
          <div className="space-y-3">
            <h2 className="font-semibold text-sm text-foreground">Input</h2>
            <ImageBox
              src={
                originalImage ? `data:image/png;base64,${originalImage}` : null
              }
              alt="Input image"
              className="w-full"
            />
          </div>

          {/* Control Panel */}
          <div className="space-y-3">
            <h2 className="font-semibold text-sm text-foreground">Controls</h2>
            <div className="space-y-4">
              <ParameterTable
                selectedMethod={selectedMethod}
                onMethodChange={setSelectedMethod}
              />
              <ActionButtons
                onApply={handleApply}
                onReset={handleReset}
                applyDisabled={!originalImage}
                loading={loading}
              />
            </div>
          </div>

          {/* Output Image */}
          <div className="space-y-3">
            <h2 className="font-semibold text-sm text-foreground">Output</h2>
            <ImageBox
              src={outputImage ? `data:image/png;base64,${outputImage}` : null}
              alt="Output image"
              className="w-full"
            />
          </div>
        </div>
      </div>
    </main>
  );
}
