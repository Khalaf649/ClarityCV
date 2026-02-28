"use client";

import { useState, useCallback, useEffect, useRef } from "react";
import { ImageBox } from "./ImageBox";
import { Button } from "./ui/Button";
import { ControlPanel } from "./ControlPanel";
import {
  loadImageToCanvas,
  toGrayscale,
  normalizeImage,
  equalizeImage,
  computeHistogram,
  cloneImageData,
  isGrayscale,
} from "@/lib/imageProcessing";

function drawHistogram(
  canvas: HTMLCanvasElement,
  histogram: ReturnType<typeof computeHistogram>,
) {
  const ctx = canvas.getContext("2d")!;
  const w = canvas.width,
    h = canvas.height;
  ctx.fillStyle = "hsl(220, 18%, 10%)";
  ctx.fillRect(0, 0, w, h);

  const drawChannel = (data: number[], color: string, alpha: number) => {
    const max = Math.max(...data);
    if (max === 0) return;
    ctx.fillStyle = color;
    ctx.globalAlpha = alpha;
    const barW = w / 256;
    for (let i = 0; i < 256; i++) {
      const barH = (data[i] / max) * (h - 20);
      ctx.fillRect(i * barW, h - barH - 10, barW, barH);
    }
    ctx.globalAlpha = 1;
  };

  const drawCurve = (data: number[], color: string) => {
    const max = Math.max(...data);
    if (max === 0) return;
    ctx.beginPath();
    ctx.strokeStyle = color;
    ctx.lineWidth = 1.5;
    const barW = w / 256;
    for (let i = 0; i < 256; i++) {
      const x = i * barW + barW / 2;
      const y = h - (data[i] / max) * (h - 20) - 10;
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
  };

  if (histogram.isGray) {
    drawChannel(histogram.gray, "hsl(0, 0%, 70%)", 0.6);
    drawCurve(histogram.gray, "hsl(0, 0%, 90%)");
  } else {
    drawChannel(histogram.r, "hsl(0, 80%, 55%)", 0.4);
    drawChannel(histogram.g, "hsl(120, 80%, 45%)", 0.4);
    drawChannel(histogram.b, "hsl(220, 80%, 55%)", 0.4);
    drawCurve(histogram.r, "hsl(0, 80%, 65%)");
    drawCurve(histogram.g, "hsl(120, 80%, 55%)");
    drawCurve(histogram.b, "hsl(220, 80%, 65%)");
  }

  // Axis labels
  ctx.globalAlpha = 1;
  ctx.fillStyle = "hsl(200, 10%, 50%)";
  ctx.font = "10px monospace";
  ctx.fillText("0", 2, h - 1);
  ctx.fillText("255", w - 25, h - 1);
}

export function HistogramTab() {
  const [imageData, setImageData] = useState<ImageData | null>(null);
  const [currentImage, setCurrentImage] = useState<ImageData | null>(null);
  const histCanvasRef = useRef<HTMLCanvasElement>(null);

  const imageIsGray = currentImage ? isGrayscale(currentImage) : false;

  const handleUpload = useCallback(async (file: File) => {
    const url = URL.createObjectURL(file);
    const { imageData: id } = await loadImageToCanvas(url);
    URL.revokeObjectURL(url);
    setImageData(id);
    setCurrentImage(id);
  }, []);

  useEffect(() => {
    if (currentImage && histCanvasRef.current) {
      const hist = computeHistogram(currentImage);
      drawHistogram(histCanvasRef.current, hist);
    }
  }, [currentImage]);

  const convertToGray = () => {
    if (!currentImage) return;
    setCurrentImage(toGrayscale(currentImage));
  };

  const normalize = () => {
    if (!currentImage) return;
    setCurrentImage(normalizeImage(currentImage));
  };

  const equalize = () => {
    if (!currentImage) return;
    setCurrentImage(equalizeImage(currentImage));
  };

  const reset = () => {
    if (imageData) setCurrentImage(cloneImageData(imageData));
  };

  return (
    <div className="flex flex-col lg:flex-row gap-6 h-full">
      <div className="flex-1 grid grid-cols-1 md:grid-cols-2 gap-4">
        <ImageBox
          title="Image"
          imageData={currentImage}
          onUpload={handleUpload}
        />
        <div className="flex flex-col gap-2">
          <span className="text-xs font-mono text-muted-foreground uppercase tracking-wider">
            Histogram {imageIsGray ? "(Grayscale)" : "(RGB)"}
          </span>
          <div className="image-box glow-border aspect-[4/3]">
            <canvas
              ref={histCanvasRef}
              width={512}
              height={300}
              className="w-full h-full"
            />
          </div>
        </div>
      </div>

      <div className="w-full lg:w-72 shrink-0">
        <ControlPanel title="Operations">
          <div className="space-y-2">
            <Button
              onClick={convertToGray}
              disabled={!currentImage || imageIsGray}
              className="w-full"
              size="sm"
              variant="secondary"
            >
              Convert to Grayscale
            </Button>
            <Button
              onClick={normalize}
              disabled={!currentImage}
              className="w-full"
              size="sm"
              variant="secondary"
            >
              Normalize
            </Button>
            <Button
              onClick={equalize}
              disabled={!currentImage}
              className="w-full"
              size="sm"
              variant="secondary"
            >
              Equalize
            </Button>
            <Button
              onClick={reset}
              disabled={!imageData}
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
