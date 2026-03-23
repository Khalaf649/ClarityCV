"use client";

import { useCallback, useEffect, useReducer } from "react";
import { ImageBox } from "@/components/ImageBox";
import { useImageContext } from "@/contexts/ImageContext";
import { api } from "@/lib/api";
import { ParametersPanel } from "./ParametersPanel";
import { ActionButtons } from "./ActionButtons";
import { StatisticsPanel } from "./StatisticsPanel";

const DEFAULT_ALPHA = 1.0;
const DEFAULT_BETA = 1.0;
const DEFAULT_GAMMA = 1.0;
const DEFAULT_ITERATIONS = 100;

interface State {
  outputImage: string | null;
  activePoints: Array<{ x: number; y: number }>;
  perimeter: number | null;
  area: number | null;
  chainCode: string | null;
  alpha: number;
  beta: number;
  gamma: number;
  iterations: number;
  loading: boolean;
  error: string | null;
}

type Action =
  | { type: "SET_OUTPUT_IMAGE"; payload: string | null }
  | { type: "ADD_POINT"; payload: { x: number; y: number } }
  | { type: "CLEAR_POINTS" }
  | { type: "SET_STATISTICS"; payload: { perimeter: number; area: number; chainCode: string } }
  | { type: "SET_ALPHA"; payload: number }
  | { type: "SET_BETA"; payload: number }
  | { type: "SET_GAMMA"; payload: number }
  | { type: "SET_ITERATIONS"; payload: number }
  | { type: "SET_LOADING"; payload: boolean }
  | { type: "SET_ERROR"; payload: string | null }
  | { type: "RESET" }
  | { type: "CLEAR_ON_IMAGE_CHANGE" };

const initialState: State = {
  outputImage: null,
  activePoints: [],
  perimeter: null,
  area: null,
  chainCode: null,
  alpha: DEFAULT_ALPHA,
  beta: DEFAULT_BETA,
  gamma: DEFAULT_GAMMA,
  iterations: DEFAULT_ITERATIONS,
  loading: false,
  error: null,
};

function reducer(state: State, action: Action): State {
  switch (action.type) {
    case "SET_OUTPUT_IMAGE":
      return { ...state, outputImage: action.payload };
    case "ADD_POINT":
      return { ...state, activePoints: [...state.activePoints, action.payload] };
    case "CLEAR_POINTS":
      return { ...state, activePoints: [] };
    case "SET_STATISTICS":
      return {
        ...state,
        perimeter: action.payload.perimeter,
        area: action.payload.area,
        chainCode: action.payload.chainCode,
      };
    case "SET_ALPHA":
      return { ...state, alpha: action.payload };
    case "SET_BETA":
      return { ...state, beta: action.payload };
    case "SET_GAMMA":
      return { ...state, gamma: action.payload };
    case "SET_ITERATIONS":
      return { ...state, iterations: action.payload };
    case "SET_LOADING":
      return { ...state, loading: action.payload };
    case "SET_ERROR":
      return { ...state, error: action.payload };
    case "RESET":
      return initialState;
    case "CLEAR_ON_IMAGE_CHANGE":
      return {
        ...state,
        outputImage: null,
        activePoints: [],
        chainCode: null,
      };
    default:
      return state;
  }
}

export function ActiveContourTab() {
  const { originalImage, setImageFromFile } = useImageContext();
  const [state, dispatch] = useReducer(reducer, initialState);
  
  // Clear output when image changes
  useEffect(() => {
    dispatch({ type: "CLEAR_ON_IMAGE_CHANGE" });
  }, [originalImage]);

  const handleUpload = useCallback(
    async (file: File) => {
      await setImageFromFile(file);
    },
    [setImageFromFile],
  );

  const handleImageClick = useCallback(
    (coords: { x: number; y: number }) => {
      dispatch({ type: "ADD_POINT", payload: coords });
    },
    [],
  );

  const handleApply = async () => {
    if (!originalImage || state.activePoints.length === 0) {
      dispatch({ type: "SET_ERROR", payload: "Please upload an image and place at least one control point" });
      return;
    }

    dispatch({ type: "SET_LOADING", payload: true });
    dispatch({ type: "SET_ERROR", payload: null });
    try {
      const res = await api.activeContour(
        originalImage,
        state.alpha,
        state.beta,
        state.gamma,
        state.iterations,
        state.activePoints,
      );
      dispatch({
        type: "SET_STATISTICS",
        payload: {
          perimeter: res.perimeter,
          area: res.area,
          chainCode: res.chainCode,
        },
      });
      dispatch({ type: "SET_OUTPUT_IMAGE", payload: res.image });
    } catch (e: unknown) {
      dispatch({
        type: "SET_ERROR",
        payload: e instanceof Error ? e.message : "Error processing active contour",
      });
    } finally {
      dispatch({ type: "SET_LOADING", payload: false });
    }
  };

  const handleReset = () => {
    dispatch({ type: "RESET" });
  };

  return (
    <div className="flex flex-col lg:flex-row gap-6 h-full">
      {/* Images */}
      <div className="flex-1 grid grid-cols-2 gap-4">
        <ImageBox
          title="Input"
          image={originalImage}
          onUpload={handleUpload}
          activePoints={state.activePoints}
          onImageClick={handleImageClick}
        />
        <ImageBox title="Output" image={state.outputImage} />
      </div>

      {/* Controls */}
      <div className="w-full lg:w-72 shrink-0 space-y-4">
        {state.error && <p className="text-xs text-red-400">{state.error}</p>}
        {state.loading && <p className="text-xs text-primary animate-pulse">Processing…</p>}
        {state.activePoints.length > 0 && (
          <p className="text-xs text-muted-foreground">
            {state.activePoints.length} control point{state.activePoints.length !== 1 ? "s" : ""} placed
            {state.activePoints.length > 1
              ? ". The initial snake follows the lines between consecutive points."
              : ""}
          </p>
        )}

        <ParametersPanel
          alpha={state.alpha}
          beta={state.beta}
          gamma={state.gamma}
          iterations={state.iterations}
          onAlphaChange={(value) => dispatch({ type: "SET_ALPHA", payload: value })}
          onBetaChange={(value) => dispatch({ type: "SET_BETA", payload: value })}
          onGammaChange={(value) => dispatch({ type: "SET_GAMMA", payload: value })}
          onIterationsChange={(value) => dispatch({ type: "SET_ITERATIONS", payload: value })}
        />

        <StatisticsPanel perimeter={state.perimeter} area={state.area} chainCode={state.chainCode} />

        <ActionButtons
          onApply={handleApply}
          onReset={handleReset}
          applyDisabled={!originalImage || state.activePoints.length === 0}
          loading={state.loading}
        />
      </div>
    </div>
  );
}
