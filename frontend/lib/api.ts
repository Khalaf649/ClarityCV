// All requests go to /api/* — Next.js rewrites them to the backend.
// This works both in dev (localhost:3000 → localhost:8080) and
// in Docker (Next.js server → backend container via internal network).

const PREFIX = "/api";

async function post<T>(endpoint: string, body: object): Promise<T> {
  const res = await fetch(`${PREFIX}${endpoint}`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });

  const data = await res.json();
  if (!data.success) throw new Error(data.error ?? "Backend error");
  return data as T;
}

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export interface ImageResponse {
  success: boolean;
  image: string;
}

export interface LoadResponse extends ImageResponse {
  width: number;
  height: number;
  channels: number;
}

export interface EdgeResponse {
  success: boolean;
  image_x: string;
  image_y: string;
  image_combined: string;
}

export interface HistogramChannel {
  label: string;
  bins: number[];
  cdf: number[];
}

export interface HistogramChannelWithStats extends HistogramChannel {
  mean: number;
  stddev: number;
}

export interface HistogramResponse {
  success: boolean;
  channels: HistogramChannel[];
  plot: string;
}

export interface HistogramCurveResponse {
  success: boolean;
  channels: HistogramChannelWithStats[];
  plot: string;        // plain histogram
  plot_curve: string;  // histogram + Gaussian distribution curve
}

export interface ThresholdResponse extends ImageResponse {
  applied_threshold: number;
}

export interface FrequencyResponse {
  success: boolean;
  image: string;
  spectrum: string;
}

export interface HybridResponse {
  success: boolean;
  low_freq_image: string;
  high_freq_image: string;
  hybrid_image: string;
}

export interface ContourPoint {
  x: number;
  y: number;
}

export interface ActiveContourResponse {
  success: boolean;
  image: string;
  points: ContourPoint[];
  perimeter: number;
  area: number;
  chainCode: string;
}

export interface CornerDetectionResponse {
  success: boolean;
  image: string;
  computationTime: number;
  featureCount: number;
}



// ---------------------------------------------------------------------------
// API calls
// ---------------------------------------------------------------------------

export const api = {
  health: () =>
    fetch(`${PREFIX}/health`).then((r) => r.json()),

  load: (image: string, mode: "rgb" | "gray" = "rgb") =>
    post<LoadResponse>("/load", { image, mode }),

  noise: (
    image: string,
    type: "gaussian" | "uniform" | "salt_pepper",
    params: {
      mean?: number;
      stddev?: number;
      low?: number;
      high?: number;
      salt_prob?: number;
      pepper_prob?: number;
    } = {},
  ) => post<ImageResponse>("/noise", { image, type, ...params }),

  filter: (
    image: string,
    type: "average" | "gaussian" | "median",
    kernel_size: number,
    sigma = 0,
  ) => post<ImageResponse>("/filter", { image, type, kernel_size, sigma }),

  edge: (
    image: string,
    type: "sobel" | "roberts" | "prewitt" | "canny",
    params: { canny_low?: number; canny_high?: number; sobel_ksize?: number } = {},
  ) => post<EdgeResponse>("/edge", { image, type, ...params }),

  histogram: (image: string) =>
    post<HistogramResponse>("/histogram", { image }),

  histogramCurve: (image: string) =>
    post<HistogramCurveResponse>("/histogram_curve", { image }),

  equalize: (image: string) =>
    post<ImageResponse>("/equalize", { image }),

  normalize: (image: string) =>
    post<ImageResponse>("/normalize", { image }),

  threshold: (
    image: string,
    type: "global_binary" | "global_otsu" | "local_mean" | "local_gaussian",
    params: { threshold?: number; block_size?: number; c?: number } = {},
  ) => post<ThresholdResponse>("/threshold", { image, type, ...params }),

  frequency: (
    image: string,
    filter_type: "low_pass" | "high_pass",
    cutoff: number,
  ) => post<FrequencyResponse>("/frequency", { image, filter_type, cutoff }),

  hybrid: (
    image1: string,
    image2: string,
    low_cutoff: number,
    high_cutoff: number,
    alpha = 0.5,
  ) =>
    post<HybridResponse>("/hybrid", {
      image1,
      image2,
      low_cutoff,
      high_cutoff,
      alpha,
    }),

  houghTransform: (
    image: string,
    shape_type: "line" | "circle" | "ellipse",
    votes_threshold: number
  ) => post<ImageResponse>("/hough_transform", { image, shape_type, votes_threshold }),

  activeContour: (
    image: string,
    alpha: number,
    beta: number,
    gamma: number,
    iterations: number,
    initial_points: ContourPoint[],
  ) =>
    post<ActiveContourResponse>("/active_contour", {
      image,
      alpha,
      beta,
      gamma,
      iterations,
      initial_points,
    }),

  cornerDetection: (
    image: string,
    mode: "Harris" | "Shi-Tomasi",
    sigma: number,
    windowSize: number,
    threshold: number,
    k: number
  ) => post<CornerDetectionResponse>("/corner_detection", { image, mode, sigma, windowSize, threshold, k }),
};