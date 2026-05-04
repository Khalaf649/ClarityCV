// All requests go to /api/* — Next.js rewrites them to the backend.
// This works both in dev (localhost:3000 → localhost:8080) and
// in Docker (Next.js server → backend container via internal network).

const PREFIX = "/api";

async function postFormData<T>(
  endpoint: string,
  formData: FormData,
): Promise<T> {
  const res = await fetch(`${PREFIX}${endpoint}`, {
    method: "POST",
    body: formData,
  });

  if (!res.ok) {
    const contentType = res.headers.get("content-type");
    if (contentType?.includes("application/json")) {
      const error = await res.json();
      throw new Error(error.detail ?? `Backend error: ${res.status}`);
    } else {
      const text = await res.text();
      throw new Error(`Backend error ${res.status}: ${text.substring(0, 200)}`);
    }
  }

  const contentType = res.headers.get("content-type");
  if (!contentType?.includes("application/json")) {
    const text = await res.text();
    throw new Error(`Invalid response type: ${contentType}, got: ${text.substring(0, 100)}`);
  }

  const data = await res.json();
  return data as T;
}

// Helper function to convert base64 to File
function base64ToFile(base64: string, filename: string, mimeType: string = "image/png"): File {
  // Handle both raw base64 and data URI formats
  let actualBase64 = base64;
  if (base64.includes(",")) {
    // This is a data URI, extract the base64 part
    actualBase64 = base64.split(",")[1];
  }

  const bstr = atob(actualBase64);
  const n = bstr.length;
  const u8arr = new Uint8Array(n);
  for (let i = 0; i < n; i++) {
    u8arr[i] = bstr.charCodeAt(i);
  }
  return new File([u8arr], filename, { type: mimeType });
}

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
  plot: string; // plain histogram
  plot_curve: string; // histogram + Gaussian distribution curve
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

export interface SIFTResponse {
  success: boolean;
  image: string;
  computationTime: number;
  featureCount: number;
}

export interface FeatureMatchingResponse {
  success: boolean;
  image: string;
  matchesCount: number;
  computationTime: number;
  siftTimeImg1: number;
  siftTimeImg2: number;
  keypointsImg1: number;
  keypointsImg2: number;
}

export interface SegmentationResponse {
  method: string;
  filename: string;
  mime_type: string;
  image_base64: string;
  metadata: Record<string, unknown>;
}

export interface FaceRecognitionResponse {
  success: boolean;
  image: string;
  facesDetected: number;
  computationTime: number;
}

// ---------------------------------------------------------------------------
// API calls
// ---------------------------------------------------------------------------

export const api = {
  health: () => fetch(`${PREFIX}/health`).then((r) => r.json()),

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
    params: {
      canny_low?: number;
      canny_high?: number;
      sobel_ksize?: number;
    } = {},
  ) => post<EdgeResponse>("/edge", { image, type, ...params }),

  histogram: (image: string) =>
    post<HistogramResponse>("/histogram", { image }),

  histogramCurve: (image: string) =>
    post<HistogramCurveResponse>("/histogram_curve", { image }),

  equalize: (image: string) => post<ImageResponse>("/equalize", { image }),

  normalize: (image: string) => post<ImageResponse>("/normalize", { image }),

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
    votes_threshold: number,
  ) =>
    post<ImageResponse>("/hough_transform", {
      image,
      shape_type,
      votes_threshold,
    }),

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
    k: number,
  ) =>
    post<CornerDetectionResponse>("/corner_detection", {
      image,
      mode,
      sigma,
      windowSize,
      threshold,
      k,
    }),

  sift: (
    image: string,
    params: {
      contrastThreshold: number;
      nfeatures: number;
    },
  ) =>
    post<SIFTResponse>("/sift", {
      image,
      contrastThreshold: params.contrastThreshold,
      nfeatures: params.nfeatures,
    }),

  featureMatching: (
    image1: string,
    image2: string,
    method: "SSD" | "NCC",
    maxMatches = 50,
    ratioThreshold = 0.75,
  ) =>
    post<FeatureMatchingResponse>("/feature_matching", {
      image1,
      image2,
      method,
      maxMatches,
      ratioThreshold,
    }),

  segmentationThresholding: (
    imageBase64: string,
    method: "kmeans" | "region_growing" | "agglomerative" | "mean_shift",
    params: Record<string, unknown> = {},
  ) => {
    const formData = new FormData();
    const file = base64ToFile(imageBase64, "image.png");
    formData.append("file", file);
    formData.append("category", "segmentation");
    formData.append("method", method);

    // Add optional parameters
    if (params.k !== undefined) formData.append("k", String(params.k));
    if (params.seed !== undefined) formData.append("seed", String(params.seed));
    if (params.include_xy !== undefined)
      formData.append("include_xy", String(params.include_xy));
    if (params.seed_x !== undefined) formData.append("seed_x", String(params.seed_x));
    if (params.seed_y !== undefined) formData.append("seed_y", String(params.seed_y));
    if (params.threshold !== undefined)
      formData.append("threshold", String(params.threshold));
    if (params.connectivity !== undefined)
      formData.append("connectivity", String(params.connectivity));
    if (params.clusters !== undefined)
      formData.append("clusters", String(params.clusters));
    if (params.sample_size !== undefined)
      formData.append("sample_size", String(params.sample_size));
    if (params.bandwidth !== undefined)
      formData.append("bandwidth", String(params.bandwidth));
    if (params.merge_radius !== undefined)
      formData.append("merge_radius", String(params.merge_radius));

    return postFormData<SegmentationResponse>("/segment", formData);
  },

  segmentationAdvanced: (
    imageBase64: string,
    method: "optimal" | "otsu" | "spectral" | "local",
    params: Record<string, unknown> = {},
  ) => {
    const formData = new FormData();
    const file = base64ToFile(imageBase64, "image.png");
    formData.append("file", file);
    formData.append("category", "thresholding");
    formData.append("method", method);

    // Add optional parameters
    if (params.epsilon !== undefined) formData.append("epsilon", String(params.epsilon));
    if (params.max_iter !== undefined) formData.append("max_iter", String(params.max_iter));
    if (params.levels !== undefined) formData.append("levels", String(params.levels));
    if (params.window_size !== undefined)
      formData.append("window_size", String(params.window_size));
    if (params.offset !== undefined) formData.append("offset", String(params.offset));

    return postFormData<SegmentationResponse>("/segment", formData);
  },

  recognizeFaces: (
    image: string,
    params: { method: string, threshold: number },
  ) => post<FaceRecognitionResponse>("/recognize_faces", { image, ...params }),
};
