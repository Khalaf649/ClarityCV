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

// Multipart upload — used by /api/segment which expects file + form fields.
// The segment response from FastAPI doesn't have a "success" boolean, so we
// rely on the HTTP status code instead of treating !data.success as failure.
async function postFormData<T>(endpoint: string, formData: FormData): Promise<T> {
  const res = await fetch(`${PREFIX}${endpoint}`, {
    method: "POST",
    body: formData,
  });

  if (!res.ok) {
    const ct = res.headers.get("content-type") ?? "";
    if (ct.includes("application/json")) {
      const err = await res.json();
      throw new Error(err.detail ?? `Backend error ${res.status}`);
    }
    const text = await res.text();
    throw new Error(`Backend error ${res.status}: ${text.slice(0, 200)}`);
  }

  return (await res.json()) as T;
}

// Convert a base64-encoded image (with or without `data:` prefix) into a File
// object so we can attach it to a multipart/form-data POST.
function base64ToFile(base64: string, filename: string, mimeType: string = "image/png"): File {
  let raw = base64;
  if (raw.includes(",")) raw = raw.split(",", 2)[1];
  const bin = atob(raw);
  const bytes = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);
  return new File([bytes], filename, { type: mimeType });
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

export interface FaceRecognitionResponse {
  success: boolean;
  image: string;
  facesDetected: number;
  computationTime: number;
}

// Segmentation API (POST /api/segment, GET /api/methods).
// Note: this endpoint comes from the merged Python service — its response
// shape doesn't include a `success` boolean. Existence of `image_base64`
// implies success; HTTP status drives error handling.
export interface SegmentationResponse {
  method: string;
  filename: string;
  mime_type: string;
  image_base64: string;
  metadata: Record<string, unknown>;
}

export interface SegmentationMethod {
  name: string;
  category: string;
  accepts: string;
  description: string;
  params: Record<string, unknown>;
}

export interface SegmentationMethodsResponse {
  thresholding: SegmentationMethod[];
  segmentation: SegmentationMethod[];
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

  recognizeFaces: (
    image: string,
    params: { method: string, threshold: number },
  ) => post<FaceRecognitionResponse>("/recognize_faces", { image, ...params }),

  // ─── Segmentation / thresholding (merged Python service) ──────────────────

  // List every available method and its parameters. Used by the segmentation
  // pages on first render to know what's possible.
  listSegmentationMethods: async (): Promise<SegmentationMethodsResponse> => {
    const res = await fetch(`${PREFIX}/methods`);
    if (!res.ok) throw new Error(`Backend error ${res.status}`);
    return (await res.json()) as SegmentationMethodsResponse;
  },

  // Run a thresholding method (otsu, optimal, spectral, local).
  segmentationThresholding: (
    imageBase64: string,
    method: "optimal" | "otsu" | "spectral" | "local",
    params: Record<string, unknown> = {},
  ) => {
    const formData = new FormData();
    formData.append("file", base64ToFile(imageBase64, "image.png"));
    formData.append("category", "thresholding");
    formData.append("method", method);
    for (const [k, v] of Object.entries(params)) {
      if (v !== undefined && v !== null) formData.append(k, String(v));
    }
    return postFormData<SegmentationResponse>("/segment", formData);
  },

  // Run a segmentation method (kmeans, region_growing, agglomerative, mean_shift).
  segmentation: (
    imageBase64: string,
    method: "kmeans" | "region_growing" | "agglomerative" | "mean_shift",
    params: Record<string, unknown> = {},
  ) => {
    const formData = new FormData();
    formData.append("file", base64ToFile(imageBase64, "image.png"));
    formData.append("category", "segmentation");
    formData.append("method", method);
    for (const [k, v] of Object.entries(params)) {
      if (v !== undefined && v !== null) formData.append(k, String(v));
    }
    return postFormData<SegmentationResponse>("/segment", formData);
  },
};
