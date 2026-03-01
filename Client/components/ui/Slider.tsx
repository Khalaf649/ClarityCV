"use client";

import { cn } from "@/lib/utils";

interface SliderProps {
  value: number[];
  onValueChange: (value: number[]) => void;
  min?: number;
  max?: number;
  step?: number;
  className?: string;
}

export function Slider({
  value,
  onValueChange,
  min = 0,
  max = 100,
  step = 1,
  className,
}: SliderProps) {
  return (
    <input
      type="range"
      min={min}
      max={max}
      step={step}
      value={value[0]}
      onChange={(e) => onValueChange([Number(e.target.value)])}
      className={cn(
        "w-full h-2 rounded-full appearance-none cursor-pointer",
        "bg-secondary accent-primary",
        "[&::-webkit-slider-thumb]:appearance-none [&::-webkit-slider-thumb]:w-4 [&::-webkit-slider-thumb]:h-4",
        "[&::-webkit-slider-thumb]:rounded-full [&::-webkit-slider-thumb]:bg-primary [&::-webkit-slider-thumb]:cursor-pointer",
        "[&::-webkit-slider-thumb]:shadow-[0_0_8px_rgba(38,193,212,0.5)]",
        "[&::-moz-range-thumb]:w-4 [&::-moz-range-thumb]:h-4 [&::-moz-range-thumb]:rounded-full",
        "[&::-moz-range-thumb]:bg-primary [&::-moz-range-thumb]:border-none [&::-moz-range-thumb]:cursor-pointer",
        className,
      )}
    />
  );
}
