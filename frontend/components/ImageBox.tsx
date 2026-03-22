"use client";

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

export function ImageBox({
  title,
  image,
  onUpload,
  className = "",
  activePoints,
  onImageClick,
}: ImageBoxProps) {
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

  const handleImageContainerClick = (e: React.MouseEvent<HTMLDivElement>) => {
    if (!src || !onImageClick) return;
    const rect = e.currentTarget.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    onImageClick({ x, y });
  };

  const src = image ? toDataUrl(image) : null;

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
            {/* eslint-disable-next-line @next/next/no-img-element */}
            <img
              src={src}
              alt={title}
              className="max-w-full max-h-full object-contain"
            />
            
            {/* Active points overlay */}
            {activePoints &&
              activePoints.map((point, idx) => (
                <div
                  key={idx}
                  className="absolute w-3 h-3 bg-red-500 rounded-full border border-red-300 pointer-events-none"
                  style={{
                    left: `${point.x}px`,
                    top: `${point.y}px`,
                    transform: "translate(-50%, -50%)",
                  }}
                  title={`Point ${idx + 1}: (${Math.round(point.x)}, ${Math.round(point.y)})`}
                />
              ))}

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
