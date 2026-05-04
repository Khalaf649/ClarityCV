"use client";

import {
  createContext,
  useContext,
  useState,
  useCallback,
  type ReactNode,
} from "react";
import { fileToBase64 } from "@/lib/imageProcessing";
// import Header from "@/components/Header";

interface ImageContextValue {
  /** Raw base64 string of the original image (no data URI prefix) */
  originalImage: string | null;
  /** Upload a file, convert to base64, and store it */
  setImageFromFile: (file: File) => Promise<void>;
  /** Programmatically set a base64 image */
  setOriginalImage: (img: string | null) => void;
  clearImage: () => void;
}

const ImageContext = createContext<ImageContextValue | null>(null);

export function ImageProvider({ children }: { children: ReactNode }) {
  const [originalImage, setOriginalImage] = useState<string | null>(null);

  const setImageFromFile = useCallback(async (file: File) => {
    const base64 = await fileToBase64(file);
    setOriginalImage(base64);
  }, []);

  const clearImage = useCallback(() => setOriginalImage(null), []);

  return (
    <ImageContext.Provider
      value={{ originalImage, setImageFromFile, setOriginalImage, clearImage }}
    >
      {/* <Header toggleSidebar={() => {}} /> */}

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
