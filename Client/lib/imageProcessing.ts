// ==================== Image Data Helpers ====================

export type ImageDataLike = {
  width: number;
  height: number;
  data: Uint8ClampedArray;
};

export function getPixel(
  img: ImageDataLike,
  x: number,
  y: number,
): [number, number, number, number] {
  const i = (y * img.width + x) * 4;
  return [img.data[i], img.data[i + 1], img.data[i + 2], img.data[i + 3]];
}

export function setPixel(
  img: ImageDataLike,
  x: number,
  y: number,
  r: number,
  g: number,
  b: number,
  a = 255,
) {
  const i = (y * img.width + x) * 4;
  img.data[i] = r;
  img.data[i + 1] = g;
  img.data[i + 2] = b;
  img.data[i + 3] = a;
}

export function cloneImageData(img: ImageData): ImageData {
  return new ImageData(new Uint8ClampedArray(img.data), img.width, img.height);
}

export function imageFromCanvas(canvas: HTMLCanvasElement): ImageData {
  const ctx = canvas.getContext("2d")!;
  return ctx.getImageData(0, 0, canvas.width, canvas.height);
}

export function loadImageToCanvas(
  src: string,
): Promise<{ canvas: HTMLCanvasElement; imageData: ImageData }> {
  return new Promise((resolve, reject) => {
    const img = new Image();
    img.crossOrigin = "anonymous";
    img.onload = () => {
      const canvas = document.createElement("canvas");
      canvas.width = img.width;
      canvas.height = img.height;
      const ctx = canvas.getContext("2d")!;
      ctx.drawImage(img, 0, 0);
      resolve({
        canvas,
        imageData: ctx.getImageData(0, 0, canvas.width, canvas.height),
      });
    };
    img.onerror = reject;
    img.src = src;
  });
}

export function putImageDataToCanvas(
  imageData: ImageData,
  canvas: HTMLCanvasElement,
) {
  canvas.width = imageData.width;
  canvas.height = imageData.height;
  const ctx = canvas.getContext("2d")!;
  ctx.putImageData(imageData, 0, 0);
}

export function isGrayscale(imageData: ImageData): boolean {
  for (let i = 0; i < imageData.data.length; i += 4) {
    if (
      imageData.data[i] !== imageData.data[i + 1] ||
      imageData.data[i + 1] !== imageData.data[i + 2]
    )
      return false;
  }
  return true;
}

// ==================== Conversion ====================

export function toGrayscale(img: ImageData): ImageData {
  const out = cloneImageData(img);
  for (let i = 0; i < out.data.length; i += 4) {
    const g = Math.round(
      0.299 * out.data[i] + 0.587 * out.data[i + 1] + 0.114 * out.data[i + 2],
    );
    out.data[i] = g;
    out.data[i + 1] = g;
    out.data[i + 2] = g;
  }
  return out;
}

// ==================== Noise ====================

function randGaussian(): number {
  let u = 0,
    v = 0;
  while (u === 0) u = Math.random();
  while (v === 0) v = Math.random();
  return Math.sqrt(-2.0 * Math.log(u)) * Math.cos(2.0 * Math.PI * v);
}

export function addUniformNoise(img: ImageData, percentage: number): ImageData {
  const out = cloneImageData(img);
  const amount = (percentage / 100) * 255;
  for (let i = 0; i < out.data.length; i += 4) {
    for (let c = 0; c < 3; c++) {
      out.data[i + c] = Math.max(
        0,
        Math.min(255, out.data[i + c] + (Math.random() - 0.5) * 2 * amount),
      );
    }
  }
  return out;
}

export function addGaussianNoise(
  img: ImageData,
  percentage: number,
): ImageData {
  const out = cloneImageData(img);
  const sigma = (percentage / 100) * 80;
  for (let i = 0; i < out.data.length; i += 4) {
    for (let c = 0; c < 3; c++) {
      out.data[i + c] = Math.max(
        0,
        Math.min(255, Math.round(out.data[i + c] + randGaussian() * sigma)),
      );
    }
  }
  return out;
}

export function addSaltPepperNoise(
  img: ImageData,
  percentage: number,
): ImageData {
  const out = cloneImageData(img);
  const prob = percentage / 100;
  for (let i = 0; i < out.data.length; i += 4) {
    const r = Math.random();
    if (r < prob / 2) {
      out.data[i] = 0;
      out.data[i + 1] = 0;
      out.data[i + 2] = 0;
    } else if (r < prob) {
      out.data[i] = 255;
      out.data[i + 1] = 255;
      out.data[i + 2] = 255;
    }
  }
  return out;
}

// ==================== Spatial Filters ====================

function convolve(
  img: ImageData,
  kernel: number[][],
  normalize = true,
): ImageData {
  const out = cloneImageData(img);
  const kh = kernel.length,
    kw = kernel[0].length;
  const hh = Math.floor(kh / 2),
    hw = Math.floor(kw / 2);
  let ksum = 0;
  if (normalize) for (const row of kernel) for (const v of row) ksum += v;
  if (ksum === 0) ksum = 1;

  for (let y = 0; y < img.height; y++) {
    for (let x = 0; x < img.width; x++) {
      const sums = [0, 0, 0];
      for (let ky = 0; ky < kh; ky++) {
        for (let kx = 0; kx < kw; kx++) {
          const px = Math.min(img.width - 1, Math.max(0, x + kx - hw));
          const py = Math.min(img.height - 1, Math.max(0, y + ky - hh));
          const idx = (py * img.width + px) * 4;
          for (let c = 0; c < 3; c++)
            sums[c] += img.data[idx + c] * kernel[ky][kx];
        }
      }
      const idx = (y * img.width + x) * 4;
      for (let c = 0; c < 3; c++) {
        out.data[idx + c] = Math.max(
          0,
          Math.min(255, Math.round(normalize ? sums[c] / ksum : sums[c])),
        );
      }
    }
  }
  return out;
}

export function averageFilter(img: ImageData, size: number): ImageData {
  const kernel = Array.from({ length: size }, () => Array(size).fill(1));
  return convolve(img, kernel);
}

export function gaussianFilter(img: ImageData, size: number): ImageData {
  const sigma = size / 6;
  const half = Math.floor(size / 2);
  const kernel: number[][] = [];
  for (let y = -half; y <= half; y++) {
    const row: number[] = [];
    for (let x = -half; x <= half; x++) {
      row.push(Math.exp(-(x * x + y * y) / (2 * sigma * sigma)));
    }
    kernel.push(row);
  }
  return convolve(img, kernel);
}

export function medianFilter(img: ImageData, size: number): ImageData {
  const out = cloneImageData(img);
  const half = Math.floor(size / 2);
  for (let y = 0; y < img.height; y++) {
    for (let x = 0; x < img.width; x++) {
      const channels: number[][] = [[], [], []];
      for (let ky = -half; ky <= half; ky++) {
        for (let kx = -half; kx <= half; kx++) {
          const px = Math.min(img.width - 1, Math.max(0, x + kx));
          const py = Math.min(img.height - 1, Math.max(0, y + ky));
          const idx = (py * img.width + px) * 4;
          for (let c = 0; c < 3; c++) channels[c].push(img.data[idx + c]);
        }
      }
      const idx = (y * img.width + x) * 4;
      for (let c = 0; c < 3; c++) {
        channels[c].sort((a, b) => a - b);
        out.data[idx + c] = channels[c][Math.floor(channels[c].length / 2)];
      }
    }
  }
  return out;
}

export function sobelFilter(img: ImageData): ImageData {
  const gray = toGrayscale(img);
  const kx = [
    [-1, 0, 1],
    [-2, 0, 2],
    [-1, 0, 1],
  ];
  const ky = [
    [-1, -2, -1],
    [0, 0, 0],
    [1, 2, 1],
  ];
  const gx = convolve(gray, kx, false);
  const gy = convolve(gray, ky, false);
  const out = cloneImageData(img);
  for (let i = 0; i < out.data.length; i += 4) {
    const mag = Math.min(255, Math.sqrt(gx.data[i] ** 2 + gy.data[i] ** 2));
    out.data[i] = mag;
    out.data[i + 1] = mag;
    out.data[i + 2] = mag;
  }
  return out;
}

export function robertsFilter(img: ImageData): ImageData {
  const gray = toGrayscale(img);
  const out = cloneImageData(img);
  for (let y = 0; y < img.height - 1; y++) {
    for (let x = 0; x < img.width - 1; x++) {
      const i00 = (y * img.width + x) * 4;
      const i01 = (y * img.width + x + 1) * 4;
      const i10 = ((y + 1) * img.width + x) * 4;
      const i11 = ((y + 1) * img.width + x + 1) * 4;
      const gx = gray.data[i00] - gray.data[i11];
      const gy = gray.data[i01] - gray.data[i10];
      const mag = Math.min(255, Math.sqrt(gx * gx + gy * gy));
      out.data[i00] = mag;
      out.data[i00 + 1] = mag;
      out.data[i00 + 2] = mag;
    }
  }
  return out;
}

export function prewittFilter(img: ImageData): ImageData {
  const gray = toGrayscale(img);
  const kx = [
    [-1, 0, 1],
    [-1, 0, 1],
    [-1, 0, 1],
  ];
  const ky = [
    [-1, -1, -1],
    [0, 0, 0],
    [1, 1, 1],
  ];
  const gx = convolve(gray, kx, false);
  const gy = convolve(gray, ky, false);
  const out = cloneImageData(img);
  for (let i = 0; i < out.data.length; i += 4) {
    const mag = Math.min(255, Math.sqrt(gx.data[i] ** 2 + gy.data[i] ** 2));
    out.data[i] = mag;
    out.data[i + 1] = mag;
    out.data[i + 2] = mag;
  }
  return out;
}

export function cannyEdgeDetector(
  img: ImageData,
  kernelSize: number,
): ImageData {
  // Simplified Canny: gaussian blur → sobel → non-max suppression → threshold
  const blurred = gaussianFilter(toGrayscale(img), kernelSize);
  const kx = [
    [-1, 0, 1],
    [-2, 0, 2],
    [-1, 0, 1],
  ];
  const ky = [
    [-1, -2, -1],
    [0, 0, 0],
    [1, 2, 1],
  ];
  const gx = convolve(blurred, kx, false);
  const gy = convolve(blurred, ky, false);

  const w = img.width,
    h = img.height;
  const magnitude = new Float32Array(w * h);
  const direction = new Float32Array(w * h);

  let maxMag = 0;
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const idx = (y * w + x) * 4;
      const mx = gx.data[idx] - 128;
      const my = gy.data[idx] - 128;
      const m = Math.sqrt(mx * mx + my * my);
      magnitude[y * w + x] = m;
      direction[y * w + x] = Math.atan2(my, mx);
      if (m > maxMag) maxMag = m;
    }
  }

  // Non-max suppression
  const suppressed = new Float32Array(w * h);
  for (let y = 1; y < h - 1; y++) {
    for (let x = 1; x < w - 1; x++) {
      const idx = y * w + x;
      const angle = ((direction[idx] * 180) / Math.PI + 180) % 180;
      let n1 = 0,
        n2 = 0;
      if (angle < 22.5 || angle >= 157.5) {
        n1 = magnitude[idx - 1];
        n2 = magnitude[idx + 1];
      } else if (angle < 67.5) {
        n1 = magnitude[(y - 1) * w + x + 1];
        n2 = magnitude[(y + 1) * w + x - 1];
      } else if (angle < 112.5) {
        n1 = magnitude[(y - 1) * w + x];
        n2 = magnitude[(y + 1) * w + x];
      } else {
        n1 = magnitude[(y - 1) * w + x - 1];
        n2 = magnitude[(y + 1) * w + x + 1];
      }
      suppressed[idx] =
        magnitude[idx] >= n1 && magnitude[idx] >= n2 ? magnitude[idx] : 0;
    }
  }

  const highThresh = maxMag * 0.15;
  const lowThresh = highThresh * 0.4;
  const out = cloneImageData(img);
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const idx = y * w + x;
      const val =
        suppressed[idx] >= highThresh
          ? 255
          : suppressed[idx] >= lowThresh
            ? 128
            : 0;
      const oi = idx * 4;
      out.data[oi] = val;
      out.data[oi + 1] = val;
      out.data[oi + 2] = val;
      out.data[oi + 3] = 255;
    }
  }
  return out;
}

// ==================== Histogram ====================

export function computeHistogram(img: ImageData): {
  r: number[];
  g: number[];
  b: number[];
  gray: number[];
  isGray: boolean;
} {
  const r = new Array(256).fill(0);
  const g = new Array(256).fill(0);
  const b = new Array(256).fill(0);
  const gray = new Array(256).fill(0);
  let isGray = true;

  for (let i = 0; i < img.data.length; i += 4) {
    r[img.data[i]]++;
    g[img.data[i + 1]]++;
    b[img.data[i + 2]]++;
    const grayVal = Math.round(
      0.299 * img.data[i] + 0.587 * img.data[i + 1] + 0.114 * img.data[i + 2],
    );
    gray[grayVal]++;
    if (img.data[i] !== img.data[i + 1] || img.data[i + 1] !== img.data[i + 2])
      isGray = false;
  }
  return { r, g, b, gray, isGray };
}

export function normalizeImage(img: ImageData): ImageData {
  const out = cloneImageData(img);
  for (let c = 0; c < 3; c++) {
    let min = 255,
      max = 0;
    for (let i = c; i < out.data.length; i += 4) {
      if (out.data[i] < min) min = out.data[i];
      if (out.data[i] > max) max = out.data[i];
    }
    const range = max - min || 1;
    for (let i = c; i < out.data.length; i += 4) {
      out.data[i] = Math.round(((out.data[i] - min) / range) * 255);
    }
  }
  return out;
}

export function equalizeImage(img: ImageData): ImageData {
  const out = cloneImageData(img);
  const totalPixels = img.width * img.height;

  for (let c = 0; c < 3; c++) {
    const hist = new Array(256).fill(0);
    for (let i = c; i < out.data.length; i += 4) hist[out.data[i]]++;
    const cdf = new Array(256).fill(0);
    cdf[0] = hist[0];
    for (let i = 1; i < 256; i++) cdf[i] = cdf[i - 1] + hist[i];
    const cdfMin = cdf.find((v) => v > 0) || 0;
    for (let i = c; i < out.data.length; i += 4) {
      out.data[i] = Math.round(
        ((cdf[out.data[i]] - cdfMin) / (totalPixels - cdfMin)) * 255,
      );
    }
  }
  return out;
}

// ==================== FFT (2D) ====================

// Simple radix-2 FFT (1D)
function fft1d(re: Float64Array, im: Float64Array, invert = false) {
  const n = re.length;
  if (n <= 1) return;

  // Bit reversal
  for (let i = 1, j = 0; i < n; i++) {
    let bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) {
      [re[i], re[j]] = [re[j], re[i]];
      [im[i], im[j]] = [im[j], im[i]];
    }
  }

  for (let len = 2; len <= n; len <<= 1) {
    const angle = ((2 * Math.PI) / len) * (invert ? -1 : 1);
    const wRe = Math.cos(angle),
      wIm = Math.sin(angle);
    for (let i = 0; i < n; i += len) {
      let curRe = 1,
        curIm = 0;
      for (let j = 0; j < len / 2; j++) {
        const uRe = re[i + j],
          uIm = im[i + j];
        const vRe = re[i + j + len / 2] * curRe - im[i + j + len / 2] * curIm;
        const vIm = re[i + j + len / 2] * curIm + im[i + j + len / 2] * curRe;
        re[i + j] = uRe + vRe;
        im[i + j] = uIm + vIm;
        re[i + j + len / 2] = uRe - vRe;
        im[i + j + len / 2] = uIm - vIm;
        const newCurRe = curRe * wRe - curIm * wIm;
        curIm = curRe * wIm + curIm * wRe;
        curRe = newCurRe;
      }
    }
  }
  if (invert) {
    for (let i = 0; i < n; i++) {
      re[i] /= n;
      im[i] /= n;
    }
  }
}

function nextPow2(n: number): number {
  let p = 1;
  while (p < n) p <<= 1;
  return p;
}

export interface FFTResult {
  re: Float64Array[];
  im: Float64Array[];
  width: number;
  height: number;
  originalWidth: number;
  originalHeight: number;
}

export function fft2d(img: ImageData): FFTResult {
  const gray = toGrayscale(img);
  const w = nextPow2(img.width);
  const h = nextPow2(img.height);

  const re: Float64Array[] = [];
  const im: Float64Array[] = [];

  // Init with image data
  for (let y = 0; y < h; y++) {
    re.push(new Float64Array(w));
    im.push(new Float64Array(w));
    for (let x = 0; x < w; x++) {
      if (y < img.height && x < img.width) {
        const idx = (y * img.width + x) * 4;
        re[y][x] = gray.data[idx] * ((x + y) % 2 === 0 ? 1 : -1); // shift
      }
    }
  }

  // FFT rows
  for (let y = 0; y < h; y++) fft1d(re[y], im[y]);
  // FFT cols
  for (let x = 0; x < w; x++) {
    const colRe = new Float64Array(h);
    const colIm = new Float64Array(h);
    for (let y = 0; y < h; y++) {
      colRe[y] = re[y][x];
      colIm[y] = im[y][x];
    }
    fft1d(colRe, colIm);
    for (let y = 0; y < h; y++) {
      re[y][x] = colRe[y];
      im[y][x] = colIm[y];
    }
  }

  return {
    re,
    im,
    width: w,
    height: h,
    originalWidth: img.width,
    originalHeight: img.height,
  };
}

export function ifft2d(fft: FFTResult): ImageData {
  const { width: w, height: h, originalWidth: ow, originalHeight: oh } = fft;
  const re = fft.re.map((r) => new Float64Array(r));
  const im = fft.im.map((r) => new Float64Array(r));

  // IFFT cols
  for (let x = 0; x < w; x++) {
    const colRe = new Float64Array(h);
    const colIm = new Float64Array(h);
    for (let y = 0; y < h; y++) {
      colRe[y] = re[y][x];
      colIm[y] = im[y][x];
    }
    fft1d(colRe, colIm, true);
    for (let y = 0; y < h; y++) {
      re[y][x] = colRe[y];
      im[y][x] = colIm[y];
    }
  }
  // IFFT rows
  for (let y = 0; y < h; y++) fft1d(re[y], im[y], true);

  const out = new ImageData(ow, oh);
  for (let y = 0; y < oh; y++) {
    for (let x = 0; x < ow; x++) {
      const val = Math.max(0, Math.min(255, Math.round(Math.abs(re[y][x]))));
      const idx = (y * ow + x) * 4;
      out.data[idx] = val;
      out.data[idx + 1] = val;
      out.data[idx + 2] = val;
      out.data[idx + 3] = 255;
    }
  }
  return out;
}

export function fftToImage(fft: FFTResult): ImageData {
  const { width: w, height: h } = fft;
  const magnitudes = new Float64Array(w * h);
  let maxLog = 0;
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const mag = Math.sqrt(fft.re[y][x] ** 2 + fft.im[y][x] ** 2);
      const logMag = Math.log(1 + mag);
      magnitudes[y * w + x] = logMag;
      if (logMag > maxLog) maxLog = logMag;
    }
  }

  const out = new ImageData(w, h);
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const val = Math.round((magnitudes[y * w + x] / (maxLog || 1)) * 255);
      const idx = (y * w + x) * 4;
      out.data[idx] = val;
      out.data[idx + 1] = val;
      out.data[idx + 2] = val;
      out.data[idx + 3] = 255;
    }
  }
  return out;
}

export function applyFrequencyFilter(
  fft: FFTResult,
  radius: number,
  type: "low" | "high",
): FFTResult {
  const { width: w, height: h } = fft;
  const cx = w / 2,
    cy = h / 2;
  const re = fft.re.map((r) => new Float64Array(r));
  const im = fft.im.map((r) => new Float64Array(r));

  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const dist = Math.sqrt((x - cx) ** 2 + (y - cy) ** 2);
      const pass =
        type === "low" ? (dist <= radius ? 1 : 0) : dist >= radius ? 1 : 0;
      re[y][x] *= pass;
      im[y][x] *= pass;
    }
  }
  return {
    re,
    im,
    width: w,
    height: h,
    originalWidth: fft.originalWidth,
    originalHeight: fft.originalHeight,
  };
}

export function createHybridImage(
  fft1: FFTResult,
  fft2: FFTResult,
  img1Mode: "low" | "high",
  img2Mode: "low" | "high",
  radius1: number,
  radius2: number,
): ImageData {
  const filtered1 = applyFrequencyFilter(fft1, radius1, img1Mode);
  const filtered2 = applyFrequencyFilter(fft2, radius2, img2Mode);

  const w = Math.min(filtered1.width, filtered2.width);
  const h = Math.min(filtered1.height, filtered2.height);
  const ow = Math.min(filtered1.originalWidth, filtered2.originalWidth);
  const oh = Math.min(filtered1.originalHeight, filtered2.originalHeight);

  const combinedRe: Float64Array[] = [];
  const combinedIm: Float64Array[] = [];
  for (let y = 0; y < h; y++) {
    combinedRe.push(new Float64Array(w));
    combinedIm.push(new Float64Array(w));
    for (let x = 0; x < w; x++) {
      combinedRe[y][x] = filtered1.re[y][x] + filtered2.re[y][x];
      combinedIm[y][x] = filtered1.im[y][x] + filtered2.im[y][x];
    }
  }
  return ifft2d({
    re: combinedRe,
    im: combinedIm,
    width: w,
    height: h,
    originalWidth: ow,
    originalHeight: oh,
  });
}
