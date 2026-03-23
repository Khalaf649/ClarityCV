"use client";

import { useCallback, useEffect, useRef, useState } from "react";
import { Upload } from "lucide-react";
import { toDataUrl } from "@/lib/imageProcessing";

interface ImageBoxProps {
  title: string;
  /** base64 string returned from backend */
  image?: string | null;
  onUpload?: (file: File) => void;
  className?: string;
  /** Points to render as overlay on the image */
  activePoints?: Array<{ x: number; y: number }>;
  /** Callback fired when the image is clicked with coordinates */
  onImageClick?: (coords: { x: number; y: number }) => void;
}

interface ImageMetrics {
  frameWidth: number;
  frameHeight: number;
  displayWidth: number;
  displayHeight: number;
  offsetX: number;
  offsetY: number;
  naturalWidth: number;
  naturalHeight: number;
}

export function ImageBox({
  title,
  image,
  onUpload,
  className = "",
  activePoints,
  onImageClick,
}: ImageBoxProps) {
  const frameRef = useRef<HTMLDivElement>(null);
  const imageRef = useRef<HTMLImageElement>(null);
  const [imageMetrics, setImageMetrics] = useState<ImageMetrics | null>(null);
  const src = image ? toDataUrl(image) : null;

  const handleClick = () => {
    if (!onUpload) return;
    const input = document.createElement("input");
    input.type = "file";
    input.accept = "image/*";
    input.onchange = (e) => {
      const file = (e.target as HTMLInputElement).files?.[0];
      if (file) onUpload(file);
    };
    input.click();
  };

  const handleDrop = (e: React.DragEvent) => {
    e.preventDefault();
    if (onUpload && e.dataTransfer.files[0]) onUpload(e.dataTransfer.files[0]);
  };

  const syncImageMetrics = useCallback(() => {
    const frameElement = frameRef.current;
    const imageElement = imageRef.current;
    if (!frameElement || !imageElement || imageElement.naturalWidth === 0 || imageElement.naturalHeight === 0) {
      return;
    }

    const frameRect = frameElement.getBoundingClientRect();
    const imageRect = imageElement.getBoundingClientRect();

    setImageMetrics({
      frameWidth: frameRect.width,
      frameHeight: frameRect.height,
      displayWidth: imageRect.width,
      displayHeight: imageRect.height,
      offsetX: imageRect.left - frameRect.left,
      offsetY: imageRect.top - frameRect.top,
      naturalWidth: imageElement.naturalWidth,
      naturalHeight: imageElement.naturalHeight,
    });
  }, []);

  useEffect(() => {
    if (!src) return;

    const frameElement = frameRef.current;
    const imageElement = imageRef.current;
    if (!frameElement || !imageElement) return;

    const observer = new ResizeObserver(() => {
      syncImageMetrics();
    });
    observer.observe(frameElement);
    observer.observe(imageElement);

    return () => {
      observer.disconnect();
    };
  }, [src, syncImageMetrics]);

  const handleImageContainerClick = (e: React.MouseEvent<HTMLDivElement>) => {
    if (!src || !onImageClick) return;

    const imageElement = imageRef.current;
    if (!imageElement || imageElement.naturalWidth === 0 || imageElement.naturalHeight === 0) {
      return;
    }

    const rect = imageElement.getBoundingClientRect();
    if (
      e.clientX < rect.left
      || e.clientX > rect.right
      || e.clientY < rect.top
      || e.clientY > rect.bottom
    ) {
      return;
    }

    const xRatio = rect.width > 0 ? (e.clientX - rect.left) / rect.width : 0;
    const yRatio = rect.height > 0 ? (e.clientY - rect.top) / rect.height : 0;

    onImageClick({
      x: Math.round(xRatio * Math.max(imageElement.naturalWidth - 1, 0)),
      y: Math.round(yRatio * Math.max(imageElement.naturalHeight - 1, 0)),
    });
  };

  const overlayPoints = activePoints ?? [];
  const displayPoints = imageMetrics
    ? overlayPoints.map((point) => ({
        x:
          imageMetrics.offsetX
          + (point.x / Math.max(imageMetrics.naturalWidth - 1, 1)) * imageMetrics.displayWidth,
        y:
          imageMetrics.offsetY
          + (point.y / Math.max(imageMetrics.naturalHeight - 1, 1)) * imageMetrics.displayHeight,
      }))
    : [];
  const overlayPath = displayPoints.map((point) => `${point.x},${point.y}`).join(" ");

  return (
    <div className={`flex flex-col gap-2 ${className}`}>
      <span className="text-xs font-mono text-muted-foreground uppercase tracking-wider">
        {title}
      </span>

      <div
        className={`image-box glow-border aspect-[4/3] relative ${
          onUpload && !src
            ? "cursor-pointer hover:border-primary transition-colors"
            : ""
        } ${onImageClick && src ? "cursor-crosshair" : ""}`}
        onDrop={handleDrop}
        onDragOver={(e) => e.preventDefault()}
        onClick={(e) => {
          if (onUpload && !src) handleClick();
          else if (onImageClick && src) handleImageContainerClick(e);
        }}
        role={onUpload && !src ? "button" : undefined}
        tabIndex={onUpload && !src ? 0 : undefined}
        aria-label={onUpload && !src ? `Upload ${title}` : undefined}
      >
        {src ? (
          <>
            <div
              ref={frameRef}
              className="relative flex h-full w-full items-center justify-center"
            >
              {/* eslint-disable-next-line @next/next/no-img-element */}
              <img
                ref={imageRef}
                src={src}
                alt={title}
                className="block max-h-full max-w-full object-contain"
                onLoad={syncImageMetrics}
              />

              {displayPoints.length > 1 && imageMetrics && (
                <svg
                  className="absolute inset-0 pointer-events-none"
                  aria-hidden="true"
                  viewBox={`0 0 ${imageMetrics.frameWidth} ${imageMetrics.frameHeight}`}
                  preserveAspectRatio="none"
                >
                  {displayPoints.length > 2 && (
                    <polygon
                      points={overlayPath}
                      fill="none"
                      stroke="rgb(45 212 191 / 0.9)"
                      strokeWidth={2}
                      vectorEffect="non-scaling-stroke"
                      strokeLinejoin="round"
                    />
                  )}
                  {displayPoints.length === 2 && (
                    <polyline
                      points={overlayPath}
                      fill="none"
                      stroke="rgb(45 212 191 / 0.9)"
                      strokeWidth={2}
                      vectorEffect="non-scaling-stroke"
                      strokeLinecap="round"
                      strokeLinejoin="round"
                    />
                  )}
                </svg>
              )}

              {displayPoints.map((point, idx) => (
                <div
                  key={`${point.x}-${point.y}-${idx}`}
                  className="absolute z-10 size-3 -translate-x-1/2 -translate-y-1/2 rounded-full border-2 border-white bg-red-500 shadow-sm pointer-events-none"
                  style={{
                    left: `${point.x}px`,
                    top: `${point.y}px`,
                  }}
                />
              ))}
            </div>

            {onUpload && (
              <button
                onClick={(e) => {
                  e.stopPropagation();
                  handleClick();
                }}
                className="absolute top-2 right-2 p-1.5 rounded-md bg-secondary/80 hover:bg-secondary text-secondary-foreground transition-colors cursor-pointer"
                aria-label="Replace image"
              >
                <Upload size={14} />
              </button>
            )}
          </>
        ) : (
          <div className="flex flex-col items-center gap-2 text-muted-foreground">
            <Upload size={24} />
            <span className="text-xs">
              {onUpload ? "Click or drop image" : "No image"}
            </span>
          </div>
        )}
      </div>
    </div>
  );
}
