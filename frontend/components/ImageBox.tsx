"use client";

import { Upload } from "lucide-react";
import { toDataUrl } from "@/lib/imageProcessing";

interface ImageBoxProps {
  title: string;
  /** base64 string returned from backend */
  image?: string | null;
  onUpload?: (file: File) => void;
  className?: string;
}

export function ImageBox({
  title,
  image,
  onUpload,
  className = "",
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
        }`}
        onDrop={handleDrop}
        onDragOver={(e) => e.preventDefault()}
        onClick={onUpload && !src ? handleClick : undefined}
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
