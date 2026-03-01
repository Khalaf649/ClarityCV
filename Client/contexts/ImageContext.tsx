"use client";

import {
  createContext,
  useContext,
  useState,
  useCallback,
  type ReactNode,
} from "react";
import { loadImageToCanvas } from "@/lib/imageProcessing";

interface ImageContextValue {
  /** The shared original image (available to Spatial, Histogram, Low/High-Pass) */
  originalImage: ImageData | null;
  /** Upload a file and store its ImageData in context */
  setImageFromFile: (file: File) => Promise<void>;
  /** Programmatically set raw ImageData */
  setOriginalImage: (img: ImageData | null) => void;
  /** Reset to empty */
  clearImage: () => void;
}

const ImageContext = createContext<ImageContextValue | null>(null);

export function ImageProvider({ children }: { children: ReactNode }) {
  const [originalImage, setOriginalImage] = useState<ImageData | null>(null);

  const setImageFromFile = useCallback(async (file: File) => {
    const url = URL.createObjectURL(file);
    try {
      const { imageData } = await loadImageToCanvas(url);
      setOriginalImage(imageData);
    } finally {
      URL.revokeObjectURL(url);
    }
  }, []);

  const clearImage = useCallback(() => {
    setOriginalImage(null);
  }, []);

  return (
    <ImageContext.Provider
      value={{ originalImage, setImageFromFile, setOriginalImage, clearImage }}
    >
      {children}
    </ImageContext.Provider>
  );
}

export function useImageContext() {
  const ctx = useContext(ImageContext);
  if (!ctx)
    throw new Error("useImageContext must be used within an ImageProvider");
  return ctx;
}
