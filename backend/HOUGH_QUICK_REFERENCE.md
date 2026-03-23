# Advanced Hough Transform - Quick Reference

## Files Modified

- **Header:** `backend/include/processing/HoughProcessor.hpp`
- **Implementation:** `backend/src/processing/HoughProcessor.cpp`
- **Documentation:** `backend/HOUGH_ADVANCED_GUIDE.md`

---

## Data Structures Added

```cpp
struct HoughLine {
    double rho, theta;    // Polar coordinates
    int votes;            // Voting strength
    cv::Point p1, p2;     // Cartesian endpoints
};

struct Circle {
    float cx, cy, radius; // Center and radius
    int votes;            // Voting strength
};
```

---

## New Parameters in HoughParams

### For Lines:
- `nmsWindowSize` (5) - NMS neighborhood size
- `lineAngleTolerance` (1.0°) - Duplicate detection angle tolerance
- `lineDistTolerance` (2.0px) - Duplicate detection distance tolerance

### For Circles:
- `minAbsVotes` (15) - Minimum absolute votes required
- `angleSampleDeg` (2.0°) - Angle sampling interval
- `circleRadiusTolerance` (0.30) - Radius duplicate tolerance
- `circleCenterDistTolerance` (1.0) - Center distance tolerance (relative to radius)

### Shared:
- `borderMargin` (5) - Pixels to ignore at borders

---

## Overlay Helper Functions

```cpp
static cv::Mat overlayLines(
    cv::Mat output,
    const std::vector<HoughLine>& lines,
    const cv::Scalar& color = cv::Scalar(0, 255, 0),
    int thickness = 2
);

static cv::Mat overlayCircles(
    cv::Mat output,
    const std::vector<Circle>& circles,
    const cv::Scalar& centerColor = cv::Scalar(0, 255, 0),
    const cv::Scalar& radiusColor = cv::Scalar(0, 0, 255),
    int thickness = 2
);
```

---

## Key Algorithms Implemented

### Hough Lines
1. ✓ Polar representation (rho, theta)
2. ✓ Precomputed sin/cos lookup tables
3. ✓ Edge-pixel-only voting with border skip
4. ✓ 2D accumulator with efficient voting
5. ✓ Configurable NMS window (5x5 default)
6. ✓ Theta wrapping for circular space
7. ✓ Sorted output by votes (descending)
8. ✓ Duplicate line removal (angle + distance)
9. ✓ Segment extraction with gap handling

### Hough Circles
1. ✓ Custom center voting (no OpenCV dependency)
2. ✓ Angle iteration (every 2° default)
3. ✓ Radius loop [minR, maxR]
4. ✓ Absolute threshold: max(minVotes, ratio × circleExpectation)
5. ✓ 3x3 Non-Maximum Suppression
6. ✓ Duplicate removal (center + radius criteria)
7. ✓ Border bias avoidance

---

## Example Usage: Detect Lines

```cpp
#include "processing/HoughProcessor.hpp"
using namespace processing;

cv::Mat image = cv::imread("image.jpg");
HoughProcessor processor;

HoughParams params;
params.shapeType = HoughShapeType::LINE;
params.threshold = 80;
params.minLineLength = 30.0;

HoughResult result = processor.apply(image, params);

// Access detections
for (const auto& line : result.lines) {
    std::cout << "Line: rho=" << line.rho 
              << ", theta=" << (line.theta * 180 / M_PI) << "°"
              << ", votes=" << line.votes << "\n";
}

cv::imshow("Result", result.transformImage);
```

---

## Example Usage: Detect Circles

```cpp
HoughParams params;
params.shapeType = HoughShapeType::CIRCLE;
params.minRadius = 20;
params.maxRadius = 150;
params.minAbsVotes = 15;
params.param2 = 20.0;

HoughResult result = processor.apply(image, params);

for (const auto& circle : result.circles) {
    std::cout << "Circle: (" << circle.cx << ", " << circle.cy 
              << "), r=" << circle.radius << "\n";
}
```

---

## Performance Tips

| Scenario | Recommended Changes |
|----------|-------------------|
| Too many false positives | ↑ threshold, ↑ cannyThreshold1, ↑ minAbsVotes |
| Missing detections | ↓ threshold, ↓ cannyThreshold1, ↓ minAbsVotes |
| Slow performance | ↑ rho, ↑ theta, ↓ angleSampleDeg, ↓ maxRadius |
| Many duplicates | ↑ lineAngleTolerance, ↑ lineDistTolerance, ↑ r_tol |
| Border artifacts | ↑ borderMargin |

---

## Backward Compatibility

Legacy formats still available in HoughResult:
- `lineSegments`: `std::vector<cv::Vec4i>` (x1,y1,x2,y2)
- `circleVec3f`: `std::vector<cv::Vec3f>` (cx,cy,r)

---

## Internal Implementation Details

### SinCosTable Struct
Pre-computes sin/cos for all theta values to avoid repeated trigonometry during voting phase.

### NMS with Theta Wrapping
```cpp
int tt = (t + dt + numTheta) % numTheta;  // Handles 0/π boundary
```

### Circle Expected Votes
```
E = 2πr / angleSampleRad
T = max(minAbsVotes, param2 × E)
```

### Duplicate Line Matching
```
Matches if: |θ₁ - θ₂| < tolerance AND |ρ₁ - ρ₂| < distTolerance
Handles wrapping: if diff > π/2, use π - diff
```

### Duplicate Circle Matching
```
Matches if: centerDist < radius × centerDistTol 
       AND |r₁ - r₂| < radius × radiusTol
```

---

## File Information

- **Total lines (implementation):** ~698
- **Header size:** ~130 lines (including documentation)
- **Implementation modular design:**
  - Helper functions in anonymous namespace (internal)
  - Public overlay functions (external API)
  - Clean separation of concerns

---

## Next Steps

1. Compile and test with sample images
2. Tune parameters for your specific use case
3. Integrate with REST API if needed
4. Consider GPU acceleration for large images (optional future work)

See `HOUGH_ADVANCED_GUIDE.md` for comprehensive documentation.
