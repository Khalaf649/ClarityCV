# Hough Transform Processing Pipeline

## Line Detection Pipeline

```
Input Image
    ↓
Convert to Grayscale
    ↓
Gaussian Blur (5×5, σ=1.5)
    ↓
Canny Edge Detection
    ├─ Lower Threshold: cannyThreshold1 (default: 50)
    └─ Upper Threshold: cannyThreshold2 (default: 150)
    ↓
[Edge Map]
    ↓
Hough Line Transform
    ├─ Skip border pixels (borderMargin = 5)
    ├─ Precompute sin/cos tables
    ├─ For each edge pixel (x, y):
    │  └─ For each angle θ:
    │     └─ Vote in accumulator at (ρ, θ)
    │        where ρ = x·cos(θ) + y·sin(θ)
    ├─ Result: 2D accumulator (numRho × numTheta)
    └─ Min threshold: threshold (default: 80 votes)
    ↓
Non-Maximum Suppression (NMS)
    ├─ Window size: nmsWindowSize (default: 5×5)
    ├─ Theta wrapping: (θ + Δθ + numTheta) % numTheta
    ├─ For each accumulator cell (ρ, θ):
    │  └─ Keep only if votes ≥ threshold AND
    │     └─ votes ≥ all neighbors in window
    └─ Result: Filtered accumulator peaks
    ↓
Extract Lines [HoughLine: ρ, θ, votes, p1, p2]
    ↓
Sort by Votes (Descending)
    ↓
Remove Duplicates
    ├─ Similarity criteria:
    │  ├─ |θ₁ - θ₂| < lineAngleTolerance (default: 1°)
    │  ├─ Handle θ∈[0,π) wrapping
    │  └─ |ρ₁ - ρ₂| < lineDistTolerance (default: 2px)
    └─ Keep highest-voted, discard duplicates
    ↓
Convert to Line Segments
    ├─ For each detected line (ρ, θ):
    │  ├─ Collect all nearby edge pixels
    │  ├─ Sort by projection along line
    │  ├─ Group into segments (maxLineGap = 10px)
    │  └─ Filter by minLineLength (default: 30px)
    └─ Result: Cartesian segments (p1, p2)
    ↓
Overlay on Output
    ├─ Color: cv::Scalar(0, 255, 0) [Green]
    ├─ Thickness: 2 pixels
    └─ Anti-aliasing: cv::LINE_AA
    ↓
Output
├─ result.lines [HoughLine vector]
├─ result.lineSegments [Vec4i for backward compatibility]
└─ result.transformImage [Annotated image]
```

---

## Circle Detection Pipeline

```
Input Image
    ↓
Convert to Grayscale
    ↓
Median Blur (5×5)
    │ Purpose: Remove salt-and-pepper noise
    │ while preserving edge sharpness
    ↓
[Blurred Grayscale]
    ↓
Canny Edge Detection
    ├─ Lower Threshold: cannyThreshold1 (default: 50)
    └─ Upper Threshold: cannyThreshold2 (default: 150)
    ↓
[Edge Map]
    ↓
Hough Circle Transform (Custom Center Voting)
    │
    ├─ Initialize accumulator (W × H)
    │
    ├─ Angle sampling: numAngles = ⌈2π / angleSampleRad⌉
    │  where angleSampleRad = angleSampleDeg * π/180
    │  default: 2° → numAngles ≈ 180
    │
    ├─ For each edge pixel (x, y):
    │  └─ For each radius r ∈ [minRadius, maxRadius]:
    │     └─ For each angle α (sampling):
    │        └─ Vote at center: (x - r·cos(α), y - r·sin(α))
    │
    └─ Result: Accumulator showing center candidates
    ↓
Extract Candidate Centers
    │ All pixels with votes > 0
    ↓
For Each Candidate: Find Best Radius
    ├─ For each radius r:
    │  └─ Count edge pixels on circle perimeter
    │
    ├─ Calculate expected votes:
    │  expectedVotes = 2π·r / angleSampleRad
    │
    ├─ Calculate adaptive threshold:
    │  threshold = max(minAbsVotes, param2 × expectedVotes)
    │  where minAbsVotes = 15 (default)
    │  where param2 = 20.0 (default)
    │
    └─ Accept if: radiusVotes ≥ threshold
    ↓
Apply 3×3 Non-Maximum Suppression
    ├─ For each pixel in accumulator:
    │  └─ Keep only if local maximum in 3×3 neighborhood
    │
    ├─ Skip border pixels (borderMargin = 5)
    │
    └─ Result: NMS-filtered candidates
    ↓
Extract Circles [Circle: cx, cy, radius, votes]
    ↓
Sort by Votes (Descending)
    ↓
Remove Duplicates
    ├─ Candidates sorted by votes (highest first)
    ├─ For each candidate:
    │  ├─ Calculate center distance to existing circles
    │  │  → dist < radius × circleCenterDistTolerance (default: 1.0×)
    │  │
    │  └─ Calculate radius difference
    │     → |r₁ - r₂| < r₁ × circleRadiusTolerance (default: 0.30)
    │
    ├─ If both criteria match → Duplicate, skip
    └─ Keep highest-voted, discard duplicates
    ↓
Overlay on Output
    ├─ Center dot (3px, green)
    ├─ Radius circle (red, thickness 2)
    └─ Anti-aliasing: cv::LINE_AA
    ↓
Output
├─ result.circles [Circle vector]
├─ result.circleVec3f [Vec3f for backward compatibility]
└─ result.transformImage [Annotated image]
```

---

## Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                       HoughProcessor::apply()                   │
│                                                                 │
│  Input: cv::Mat image, HoughParams params                       │
│  Output: HoughResult result                                     │
└─────────────────────────────────────────────────────────────────┘
                            ↓
                  ┌─────────┴─────────┐
                  ↓                   ↓
        [LINE_DETECTION]      [CIRCLE_DETECTION]
                  ↓                   ↓
            applyLine()          applyCircle()
                  ↓                   ↓
    ┌─────────────┴─────────────┐   (separate branch)
    ↓                           ↓
houghLinesAdvanced()    linesToSegments()
    ↓                           ↓
removeDuplicateLines()     HoughLine[] with p1,p2
    ↓
overlayLines()
    ↓
HoughResult::lines filled
HoughResult::lineSegments filled (legacy)
HoughResult::transformImage modified
```

---

## Algorithm Complexity Comparison

### Previous Implementation vs. Advanced

| Aspect | Previous | Advanced |
|--------|----------|----------|
| **Line NMS** | 3×3 simple | 5×5 configurable, theta wrapping |
| **Line duplicates** | Not handled | Angle + distance based |
| **Circle method** | OpenCV HoughCircles | Custom center voting |
| **Circle threshold** | Static/simple | Radius-aware absolute formula |
| **Circle NMS** | From OpenCV | Custom 3×3 |
| **Circle duplicates** | Limited | Multi-criterion matching |
| **Border handling** | Not present | Configurable margin |
| **Output format** | Vec3f/Vec4i only | HoughLine/Circle structs + legacy |
| **Sorting** | Not present | By votes (descending) |

---

## Parameter Sensitivity Analysis

### Effect on Line Detection

```
threshold ↑  →  Fewer, higher-confidence lines
             →  Fewer false positives
             →  May miss weak lines

nmsWindowSize ↑  →  More aggressive suppression
                  →  Fewer duplicate peaks
                  →  Smoother results but may merge nearby lines

lineAngleTolerance ↑  →  More aggressive duplicate removal
                       →  Parallel lines more likely grouped

lineDistTolerance ↑   →  More permissive distance matching
                       →  Lines at different ρ may be grouped
```

### Effect on Circle Detection

```
minAbsVotes ↑  →  Only strong circles detected
              →  False positives eliminated
              →  Small circles may be missed

param2 ↓  →  Threshold = param2 × expectedVotes
            →  param2 × π × r / angle_sample
            →  Smaller circles more likely to pass

minRadius ↑  →  Skip small circles entirely
              →  Faster processing

maxRadius ↓  →  Reduce search space
              →  Much faster processing
              →  May miss large circles
```

---

## Memory Layout During Execution

### Line Detection Accumulator
```
[0,0]                                    [numRho-1, 0]
  ↓                                               ↓
┌──────────────────────────────────────────────┐
│                 Accumulator                  │ ↑
│         cv::Mat (numRho × numTheta)          │ | numTheta
│       Each cell = int (4 bytes)              │ |
│                                              │ ↓
└──────────────────────────────────────────────┘
   ← numRho →
   
Typical: 512 × 180 × 4 bytes = ~368 KB
```

### Circle Detection Accumulator
```
(0,0) ............ (W-1, 0)
  .                    .
  .    Accumulator     .
  .   (W × H × 4B)     .
  .                    .
(0,H-1) ........ (W-1, H-1)

Typical: 640 × 480 × 4 bytes = ~1.2 MB
Additional: bestRadius(W×H), bestVotes(W×H), centerVotes(W×H)
Total typical: ~4.8 MB
```

---

## Voting Example: Line Detection

Suppose we detect a vertical line at x=100:

```
Vertical line: θ = π/2 (90°), ρ = 100

For each edge pixel (100, y) and all angles θ:
  ρ_computed = 100·cos(θ) + y·sin(θ)
  
At θ = π/2:
  ρ = 100·cos(π/2) + y·sin(π/2)
  ρ = 100·0 + y·1
  ρ = y
  
At θ = 0:
  ρ = 100·cos(0) + y·sin(0)
  ρ = 100·1 + y·0
  ρ = 100

Accumulator receives votes at:
  [ρ_computed][θ_index] += 1
```

---

## Voting Example: Circle Detection

Suppose we have edge pixel (200, 150) and want to detect circle at (180, 130) with r=25:

```
Vector from pixel to center: (180-200, 130-150) = (-20, -20)
Distance: sqrt(400+400) ≈ 28.3 (close to r=25)

For r=25 and angles α = {0°, 2°, 4°, ...}:
  α = 0°:   center ≈ (200 - 25·cos(0°), 150 - 25·sin(0°))
           = (200 - 25, 150 - 0) = (175, 150)
           accumulator[150][175] += 1

  α = 2°:   center ≈ (200 - 25·cos(2°), 150 - 25·sin(2°))
           ≈ (200 - 24.96, 150 - 0.87) ≈ (175.04, 149.13)
           → rounds to (175, 149)
           accumulator[149][175] += 1

  α = 90°:  center = (200 - 25·cos(90°), 150 - 25·sin(90°))
           = (200 - 0, 150 - 25) = (200, 125)
           accumulator[125][200] += 1
           
  ...
```

The true center (180, 130) receives many votes across all angles, creating a strong peak.

---

## Performance Tuning Quick Guide

### For Real-Time Processing (< 100ms)

Lines:
```cpp
params.rho = 2.0;           // Coarser resolution
params.theta = CV_PI / 90.0; // Every 2° instead of 1°
params.nmsWindowSize = 3;    // Smaller NMS
params.threshold = 150;       // Higher threshold = fewer candidates
```

Circles:
```cpp
params.minRadius = 30;
params.maxRadius = 100;
params.angleSampleDeg = 4.0;  // Sample every 4° instead of 2°
params.minAbsVotes = 25;      // Higher threshold
```

### For High Accuracy

Lines:
```cpp
params.rho = 0.5;            // Fine resolution
params.theta = CV_PI / 360.0; // Every 0.5°
params.nmsWindowSize = 7;     // Larger NMS
params.threshold = 50;        // Lower threshold
params.lineAngleTolerance = 0.5;  // Stricter
params.lineDistTolerance = 1.0;   // Stricter
```

Circles:
```cpp
params.minRadius = 5;
params.maxRadius = 300;
params.angleSampleDeg = 1.0;   // Sample every 1°
params.minAbsVotes = 10;       // Lower threshold
params.param2 = 30.0;          // More permissive relative threshold
```

---

## Debugging Checklist

If results are unexpected:

1. **No shapes detected?**
   - Lower threshold values
   - Check edge detection (Canny thresholds)
   - Verify borderMargin isn't too large
   - Check minRadius/maxRadius/minLineLength bounds

2. **Too many false positives?**
   - Raise threshold values
   - Increase Canny thresholds
   - Increase borderMargin

3. **Duplicate shapes?**
   - Increase angle/distance tolerances (lines)
   - Increase radius/center tolerances (circles)

4. **Slow processing?**
   - Increase rho/theta step sizes
   - Decrease angleSampleDeg
   - Reduce maxRadius
   - Increase threshold to reduce NMS overhead

5. **Shapes at image border?**
   - Increase borderMargin to avoid edge artifacts

---

End of Pipeline Documentation
