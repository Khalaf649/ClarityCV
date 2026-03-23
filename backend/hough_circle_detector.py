"""
Weighted Hough Circle Detector
-------------------------------
Detects circles in an image using:
  - Canny edge detection (via OpenCV)
  - Improved Hough voting:
      * Magnitude-weighted votes (strong edges vote harder)
      * Angular sweep +-5 degrees (fills quantization gaps)
      * Gaussian 3x3 spread on accumulator (smoother peaks)
  - Non-maximum suppression to remove duplicate centers
  - Radius estimation via 1D distance histogram per center

Usage:
    python hough_circle_detector.py --image path/to/image.jpg
    python hough_circle_detector.py --image path/to/image.jpg --threshold 10
    python hough_circle_detector.py --image path/to/image.jpg --threshold 10 --min-radius 15 --max-radius 100 --save result.jpg
"""

import cv2
import numpy as np
import argparse
import sys
from pathlib import Path


# ── Constants ─────────────────────────────────────────────────────────────────

# Gaussian 3x3 kernel (sigma ~0.8), weights sum to ~1
GK = np.array([
    [0.07, 0.12, 0.07],
    [0.12, 0.24, 0.12],
    [0.07, 0.12, 0.07]
], dtype=np.float32)

# Angular sweep offsets: +-5 degrees in radians
ANGLE_OFFSETS = np.array([-0.087, -0.044, 0.0, 0.044, 0.087], dtype=np.float32)


# ── Stage 1: Preprocessing ────────────────────────────────────────────────────

def preprocess(image: np.ndarray) -> tuple:
    """
    Convert to grayscale, blur, compute Sobel gradients and Canny edges.

    Returns:
        gray    : blurred grayscale image
        mag     : gradient magnitude (float32, normalized 0-1)
        ang     : gradient angle in radians (float32)
        edges   : Canny binary edge map (uint8, 0 or 255)
    """
    # Grayscale
    if len(image.shape) == 3:
        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    else:
        gray = image.copy()

    # Blur to reduce noise
    blurred = cv2.GaussianBlur(gray, (5, 5), 1.5)

    # Sobel gradients
    gx = cv2.Sobel(blurred, cv2.CV_32F, 1, 0, ksize=3)
    gy = cv2.Sobel(blurred, cv2.CV_32F, 0, 1, ksize=3)

    mag = np.sqrt(gx**2 + gy**2)
    ang = np.arctan2(gy, gx).astype(np.float32)

    # Normalize magnitude to 0-1
    max_mag = mag.max()
    if max_mag > 0:
        mag = (mag / max_mag).astype(np.float32)

    # Canny edges (using OpenCV directly as allowed)
    edges = cv2.Canny(blurred, 30, 80)

    return blurred, mag, ang, edges


# ── Stage 2: Improved Weighted Voting ────────────────────────────────────────

def weighted_vote(edges: np.ndarray,
                  mag: np.ndarray,
                  ang: np.ndarray,
                  min_r: int,
                  max_r: int) -> np.ndarray:
    """
    Cast weighted votes into a 2D accumulator for circle center candidates.

    For every edge pixel:
      1. Weight = normalized gradient magnitude (strong edges vote harder)
      2. Angular sweep +-5deg to fill quantization gaps at small radii
      3. Vote in both +/- gradient directions
      4. Spread each vote across a 3x3 Gaussian neighborhood

    Args:
        edges : binary edge map (uint8, 255 = edge)
        mag   : gradient magnitude normalized 0-1 (float32)
        ang   : gradient angle in radians (float32)
        min_r : minimum circle radius in pixels
        max_r : maximum circle radius in pixels

    Returns:
        acc : 2D float accumulator (same shape as edges)
    """
    h, w = edges.shape
    acc = np.zeros((h, w), dtype=np.float32)

    # Get edge pixel coordinates
    ey, ex = np.where(edges == 255)
    if len(ex) == 0:
        return acc

    edge_mag = mag[ey, ex]   # weight per edge pixel
    edge_ang = ang[ey, ex]   # gradient angle per edge pixel

    for r in range(min_r, max_r + 1):
        for da in ANGLE_OFFSETS:
            swept_ang = edge_ang + da

            for sign in (-1, 1):
                # Candidate center coordinates for this radius + angle
                cx = np.round(ex + sign * r * np.cos(swept_ang)).astype(np.int32)
                cy = np.round(ey + sign * r * np.sin(swept_ang)).astype(np.int32)

                # Spread vote across 3x3 Gaussian neighborhood
                for dy in range(-1, 2):
                    for dx in range(-1, 2):
                        nx = cx + dx
                        ny = cy + dy

                        # Mask valid coordinates
                        valid = (nx >= 0) & (nx < w) & (ny >= 0) & (ny < h)

                        gk_weight = GK[dy + 1, dx + 1]

                        # Accumulate: weight * gaussian_kernel_value
                        np.add.at(acc, (ny[valid], nx[valid]),
                                  edge_mag[valid] * gk_weight)

    return acc


# ── Stage 3: Peak Detection (Non-Maximum Suppression) ─────────────────────────

def find_peaks(acc: np.ndarray,
               threshold: float,
               min_dist: int) -> list:
    """
    Find local maxima in the accumulator above threshold,
    enforcing a minimum distance between peaks.

    Args:
        acc       : 2D float accumulator
        threshold : minimum vote score to be a candidate center
        min_dist  : minimum pixel distance between two circle centers

    Returns:
        List of (cx, cy) tuples sorted by vote score descending
    """
    h, w = acc.shape

    # Flatten and sort indices by vote score descending
    flat_indices = np.argsort(acc.ravel())[::-1]
    visited = np.zeros((h, w), dtype=bool)
    peaks = []

    for idx in flat_indices:
        cy, cx = divmod(int(idx), w)
        score = acc[cy, cx]

        if score < threshold:
            break  # sorted, so all remaining are below threshold

        if visited[cy, cx]:
            continue

        peaks.append((cx, cy, score))

        # Suppress neighborhood
        y0 = max(0, cy - min_dist)
        y1 = min(h, cy + min_dist + 1)
        x0 = max(0, cx - min_dist)
        x1 = min(w, cx + min_dist + 1)
        visited[y0:y1, x0:x1] = True

        if len(peaks) >= 40:
            break

    return peaks


# ── Stage 4: Radius Estimation ────────────────────────────────────────────────

def estimate_radius(cx: int,
                    cy: int,
                    edges: np.ndarray,
                    min_r: int,
                    max_r: int) -> int:
    """
    For a given center (cx, cy), find the best radius by building a
    1D histogram of distances from the center to all edge pixels,
    then picking the bin with the most votes.

    Args:
        cx, cy : circle center candidate
        edges  : binary edge map
        min_r  : minimum radius to consider
        max_r  : maximum radius to consider

    Returns:
        best_r : estimated radius (pixels)
    """
    ey, ex = np.where(edges == 255)
    if len(ex) == 0:
        return min_r

    dists = np.round(np.sqrt((ex - cx)**2 + (ey - cy)**2)).astype(np.int32)

    # Build histogram over [min_r, max_r]
    hist = np.zeros(max_r + 1, dtype=np.int32)
    mask = (dists >= min_r) & (dists <= max_r)
    np.add.at(hist, dists[mask], 1)

    best_r = int(np.argmax(hist[min_r:max_r + 1])) + min_r
    return best_r


# ── Stage 5: Full Detection Pipeline ─────────────────────────────────────────

def detect_circles(image: np.ndarray,
                   threshold: float = 4.0,
                   min_radius: int = 10,
                   max_radius: int = None,
                   min_dist: int = 20) -> list:
    """
    Full circle detection pipeline.

    Args:
        image      : input BGR or grayscale image
        threshold  : accumulator vote threshold (lower = more circles)
        min_radius : minimum circle radius in pixels
        max_radius : maximum circle radius in pixels (default: half of min dimension)
        min_dist   : minimum distance between circle centers in pixels

    Returns:
        List of dicts: [{'cx': int, 'cy': int, 'r': int, 'score': float}, ...]
    """
    h, w = image.shape[:2]

    if max_radius is None:
        max_radius = min(min(h, w) // 2, 150)

    max_radius = max(max_radius, min_radius + 1)

    # Stage 1 — preprocess
    _, mag, ang, edges = preprocess(image)

    # Stage 2 — weighted voting
    print(f"  Voting... (edge pixels: {np.sum(edges == 255)}, "
          f"radius range: {min_radius}-{max_radius}px)")
    acc = weighted_vote(edges, mag, ang, min_radius, max_radius)

    peak_val = acc.max()
    print(f"  Peak accumulator value: {peak_val:.2f}")

    # Stage 3 — find peaks
    peaks = find_peaks(acc, threshold, min_dist)
    print(f"  Candidate centers found: {len(peaks)}")

    # Stage 4 — estimate radius for each peak
    circles = []
    for cx, cy, score in peaks:
        r = estimate_radius(cx, cy, edges, min_radius, max_radius)
        circles.append({'cx': cx, 'cy': cy, 'r': r, 'score': score})

    print(f"  Circles after radius estimation: {len(circles)}")
    return circles


# ── Drawing ───────────────────────────────────────────────────────────────────

def draw_circles(image: np.ndarray, circles: list) -> np.ndarray:
    """
    Draw detected circles on a copy of the image.

    Args:
        image   : input image (BGR)
        circles : list of circle dicts from detect_circles()

    Returns:
        annotated image copy
    """
    output = image.copy()
    if len(output.shape) == 2:
        output = cv2.cvtColor(output, cv2.COLOR_GRAY2BGR)

    for c in circles:
        cx, cy, r = int(c['cx']), int(c['cy']), int(c['r'])
        # Outer circle — teal green
        cv2.circle(output, (cx, cy), r, (29, 158, 117), 2, cv2.LINE_AA)
        # Center dot — red
        cv2.circle(output, (cx, cy), 3, (66, 75, 226), -1, cv2.LINE_AA)

    return output


# ── CLI ───────────────────────────────────────────────────────────────────────

def parse_args():
    parser = argparse.ArgumentParser(
        description='Weighted Hough Circle Detector',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python hough_circle_detector.py --image coins.jpg
  python hough_circle_detector.py --image coins.jpg --threshold 8 --save result.jpg
  python hough_circle_detector.py --image coins.jpg --min-radius 20 --max-radius 120
        """
    )
    parser.add_argument('--image',      required=True,       help='Path to input image')
    parser.add_argument('--threshold',  type=float, default=4.0,
                        help='Vote threshold (default: 4.0). Lower = more circles.')
    parser.add_argument('--min-radius', type=int,   default=10,
                        help='Minimum circle radius in pixels (default: 10)')
    parser.add_argument('--max-radius', type=int,   default=None,
                        help='Maximum circle radius in pixels (default: auto)')
    parser.add_argument('--min-dist',   type=int,   default=20,
                        help='Min distance between circle centers (default: 20)')
    parser.add_argument('--save',       default=None,
                        help='Save result image to this path')
    parser.add_argument('--no-display', action='store_true',
                        help='Skip displaying the result window')
    return parser.parse_args()


def main():
    args = parse_args()

    # Load image
    img_path = Path(args.image)
    if not img_path.exists():
        print(f"Error: image not found: {img_path}")
        sys.exit(1)

    image = cv2.imread(str(img_path))
    if image is None:
        print(f"Error: could not read image: {img_path}")
        sys.exit(1)

    print(f"\nImage: {img_path.name}  ({image.shape[1]}x{image.shape[0]})")
    print(f"Threshold: {args.threshold}  |  Radius: {args.min_radius}-"
          f"{args.max_radius or 'auto'}px  |  Min dist: {args.min_dist}px\n")

    # Detect
    circles = detect_circles(
        image,
        threshold=args.threshold,
        min_radius=args.min_radius,
        max_radius=args.max_radius,
        min_dist=args.min_dist
    )

    print(f"\nDetected {len(circles)} circle(s):")
    for i, c in enumerate(circles):
        print(f"  [{i+1}] center=({c['cx']}, {c['cy']})  radius={c['r']}px  score={c['score']:.2f}")

    # Draw result
    result = draw_circles(image, circles)

    # Save
    if args.save:
        cv2.imwrite(args.save, result)
        print(f"\nSaved to: {args.save}")

    # Display
    if not args.no_display:
        cv2.imshow('Detected Circles', result)
        print("\nPress any key to close...")
        cv2.waitKey(0)
        cv2.destroyAllWindows()


if __name__ == '__main__':
    main()
