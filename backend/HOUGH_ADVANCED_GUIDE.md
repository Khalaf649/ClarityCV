# Advanced Hough Transform Implementation Guide

## Overview

This document describes the advanced Hough line and circle detection implementation for the ClarityCV backend. The implementation provides professional-grade shape detection with sophisticated algorithms and fine-grained parameter control.

---

## Key Improvements Over Previous Implementation

### Hough Lines

1. **Advanced Non-Maximum Suppression (NMS)**
   - Configurable window size (default 5x5)
   - Proper theta wrapping for circular angle space
   - Efficiently removes local noise in accumulator space

2. **Intelligent Duplicate Removal**
   - Angle tolerance: Groups similar-angled lines (default: 1.0°)
   - Distance tolerance: Groups parallel lines at similar distances (default: 2.0 pixels)
   - Preserves multiple distinct lines while removing duplicates

3. **Efficient Voting**
   - Precomputed sin/cos tables (SinCosTable struct)
   - Polar representation (rho, theta) for robustness
   - Single-pass voting with edge pixels only
   - Border margin handling (default: 5 pixels)

4. **Sorted Output**
   - Lines ranked by accumulator votes (voting strength)
   - Descending order for easy filtering by confidence

5. **Segment Extraction**
   - Converts polar lines to Cartesian line segments
   - Gap-based segment grouping
   - Minimum length filtering

### Hough Circles

1. **Custom Center Voting Method**
   - No dependency on OpenCV's HoughCircles
   - Angle sampling every 2 degrees (configurable)
   - Efficient radius iteration

2. **Absolute Threshold Formula**
   ```
   expectedVotes = 2 * PI * radius / angleSampleRad
   thresholdVotes = max(minAbsVotes, param2 * expectedVotes)
   ```
   - Physical interpretation: votes relative to circle circumference
   - Radius-aware threshold: smaller circles require fewer votes

3. **3x3 Non-Maximum Suppression**
   - Local maxima detection
   - Efficient single-pass NMS

4. **Sophisticated Duplicate Removal**
   - Centers within radius distance (default: 1.0 × radius)
   - Radii differing by < 30% (default)
   - Keeps circles with highest votes

5. **Border Bias Avoidance**
   - Ignores pixels near image borders
   - Configurable border margin (default: 5 pixels)

---

## Data Structures

### HoughLine

```cpp
struct HoughLine {
    double rho;        // Perpendicular distance from origin [-maxRho, +maxRho]
    double theta;      // Angle in radians [0, PI)
    int votes;         // Accumulator count (voting strength)
    cv::Point p1, p2;  // Line segment endpoints (in Cartesian space)
};
```

**Usage:**
```cpp
HoughLine line = result.lines[0];
std::cout << "Line at rho=" << line.rho << ", theta=" << line.theta 
          << " with " << line.votes << " votes\n";
std::cout << "Segment from (" << line.p1.x << "," << line.p1.y 
          << ") to (" << line.p2.x << "," << line.p2.y << ")\n";
```

### Circle

```cpp
struct Circle {
    float cx, cy;      // Center coordinates
    float radius;      // Radius in pixels
    int votes;         // Accumulator count (voting strength)
};
```

**Usage:**
```cpp
Circle circle = result.circles[0];
std::cout << "Circle at (" << circle.cx << "," << circle.cy 
          << ") with radius=" << circle.radius 
          << " and " << circle.votes << " votes\n";
```

---

## HoughParams Configuration

### Lines-Specific Parameters

```cpp
HoughParams params;
params.shapeType = HoughShapeType::LINE;

// Edge detection
params.cannyThreshold1 = 50.0;    // Lower Canny threshold
params.cannyThreshold2 = 150.0;   // Upper Canny threshold

// Hough accumulator
params.rho = 1.0;                 // Rho resolution (pixels)
params.theta = CV_PI / 180.0;     // Theta resolution (radians, 1°)
params.threshold = 80;            // Minimum votes to detect line

// Advanced NMS and filtering
params.nmsWindowSize = 5;         // NMS neighborhood (5x5)
params.lineAngleTolerance = 1.0;  // Duplicate removal: angle tolerance (degrees)
params.lineDistTolerance = 2.0;   // Duplicate removal: distance tolerance (pixels)

// Segment extraction
params.minLineLength = 30.0;      // Minimum segment length (pixels)
params.maxLineGap = 10.0;         // Maximum gap in segment (pixels)

// Border handling
params.borderMargin = 5;          // Ignore N pixels from border
```

### Circles-Specific Parameters

```cpp
HoughParams params;
params.shapeType = HoughShapeType::CIRCLE;

// Edge detection
params.cannyThreshold1 = 50.0;
params.cannyThreshold2 = 150.0;

// Hough accumulator
params.minRadius = 10;            // Minimum circle radius (pixels)
params.maxRadius = 200;           // Maximum circle radius (pixels)
params.angleSampleDeg = 2.0;      // Angle sampling interval (degrees)

// Threshold strategy
params.minAbsVotes = 15;          // Minimum absolute votes
params.param2 = 20.0;             // Threshold ratio multiplier

// Duplicate removal
params.circleRadiusTolerance = 0.30;        // 30% tolerance on radius
params.circleCenterDistTolerance = 1.0;     // 1.0 × radius for center distance

// Border handling
params.borderMargin = 5;
```

---

## Usage Examples

### Example 1: Detect Lines from Image

```cpp
#include "processing/HoughProcessor.hpp"
using namespace processing;

cv::Mat image = cv::imread("input.jpg");
HoughProcessor processor;

// Configure parameters
HoughParams params;
params.shapeType = HoughShapeType::LINE;
params.threshold = 80;
params.cannyThreshold1 = 50.0;
params.cannyThreshold2 = 150.0;
params.minLineLength = 30.0;
params.nmsWindowSize = 5;
params.lineAngleTolerance = 1.0;  // Duplicate lines must differ by > 1°
params.lineDistTolerance = 2.0;   // or be > 2 pixels apart

// Run detection
HoughResult result = processor.apply(image, params);

// Access results
std::cout << "Detected " << result.lines.size() << " lines\n";

for (size_t i = 0; i < std::min(size_t(5), result.lines.size()); ++i) {
    const HoughLine& line = result.lines[i];
    std::cout << "  Line " << i << ": rho=" << line.rho << ", theta=" << (line.theta * 180 / M_PI)
              << "°, votes=" << line.votes << "\n";
    std::cout << "    Segment: (" << line.p1.x << "," << line.p1.y << ") -> ("
              << line.p2.x << "," << line.p2.y << ")\n";
}

// Display result
cv::imshow("Detected Lines", result.transformImage);
cv::waitKey(0);
```

### Example 2: Detect Circles from Image

```cpp
#include "processing/HoughProcessor.hpp"
using namespace processing;

cv::Mat image = cv::imread("input.jpg");
HoughProcessor processor;

// Configure parameters
HoughParams params;
params.shapeType = HoughShapeType::CIRCLE;
params.minRadius = 20;
params.maxRadius = 150;
params.minAbsVotes = 15;
params.param2 = 20.0;  // Vote threshold = max(15, 20 * expectedVotes)
params.angleSampleDeg = 2.0;  // Sample every 2°
params.borderMargin = 10;

// Run detection
HoughResult result = processor.apply(image, params);

// Access results
std::cout << "Detected " << result.circles.size() << " circles\n";

for (const auto& circle : result.circles) {
    std::cout << "  Circle: center=(" << circle.cx << "," << circle.cy << "), "
              << "radius=" << circle.radius << ", votes=" << circle.votes << "\n";
}

// Display result
cv::imshow("Detected Circles", result.transformImage);
cv::waitKey(0);
```

### Example 3: Custom Overlay with Styling

```cpp
// Detect lines
HoughResult result = processor.apply(image, lineParams);

// Create custom overlay
cv::Mat output = image.clone();
output = HoughProcessor::overlayLines(
    output,
    result.lines,
    cv::Scalar(0, 255, 0),  // Green
    3                       // Thickness
);

// Detect circles
result = processor.apply(image, circleParams);
output = HoughProcessor::overlayCircles(
    output,
    result.circles,
    cv::Scalar(0, 255, 0),  // Green centers
    cv::Scalar(0, 0, 255),  // Red radii
    2
);

cv::imshow("Combined Detection", output);
```

### Example 4: Filter Results by Confidence

```cpp
HoughResult result = processor.apply(image, params);

// Keep only high-confidence lines
int minVotes = 100;
std::vector<HoughLine> filteredLines;
for (const auto& line : result.lines) {
    if (line.votes >= minVotes) {
        filteredLines.push_back(line);
    }
}

std::cout << "High-confidence lines: " << filteredLines.size() 
          << " out of " << result.lines.size() << "\n";

// Create overlay with top-5 lines
std::vector<HoughLine> topLines(
    filteredLines.begin(),
    filteredLines.begin() + std::min(size_t(5), filteredLines.size())
);
cv::Mat output = HoughProcessor::overlayLines(image.clone(), topLines);
```

### Example 5: Backward Compatibility

```cpp
// If you need the legacy Vec3f/Vec4i formats:
HoughResult result = processor.apply(image, params);

// Lines
std::vector<cv::Vec4i>& legacyLines = result.lineSegments;
for (const auto& seg : legacyLines) {
    cv::line(output, cv::Point(seg[0], seg[1]), 
                     cv::Point(seg[2], seg[3]), 
             cv::Scalar(0, 255, 0), 2);
}

// Circles
std::vector<cv::Vec3f>& legacyCircles = result.circleVec3f;
for (const auto& c : legacyCircles) {
    cv::circle(output, cv::Point(c[0], c[1]), c[2], 
               cv::Scalar(0, 0, 255), 2);
}
```

---

## Parameter Tuning Guide

### For Detecting Long Lines (e.g., Road Edges)

```cpp
params.threshold = 120;           // Require many votes
params.nmsWindowSize = 7;         // Larger NMS window
params.minLineLength = 50.0;      // Filter short segments
params.cannyThreshold1 = 30.0;    // More permissive edge detection
params.cannyThreshold2 = 100.0;
```

### For Detecting Small Circles (e.g., Coins)

```cpp
params.minRadius = 5;
params.maxRadius = 50;
params.minAbsVotes = 8;           // Fewer votes needed (circumference of small circle)
params.param2 = 15.0;
params.angleSampleDeg = 3.0;      // Coarser sampling (faster)
```

### For Detecting Large Circles (e.g., Wheels)

```cpp
params.minRadius = 50;
params.maxRadius = 300;
params.minAbsVotes = 30;          // More votes needed
params.param2 = 25.0;
params.angleSampleDeg = 1.5;      // Finer sampling (slower but more accurate)
```

### For Noise-Robust Detection

```cpp
params.borderMargin = 10;         // Ignore border artifacts
params.nmsWindowSize = 7;         // Aggressive NMS
params.lineAngleTolerance = 2.0;  // More permissive duplicate removal
params.lineDistTolerance = 5.0;
params.circleRadiusTolerance = 0.50;  // 50% tolerance
```

---

## Performance Characteristics

### Time Complexity

- **Hough Lines**: O(W × H × numTheta)
  - Edge detection: O(W × H)
  - Voting: O(W × H × numTheta)
  - NMS: O(numRho × numTheta × nmsWindow²)
  - Segment extraction: O(numEdges × numLines)

- **Hough Circles**: O(W × H × numAngles × numRadii)
  - Edge detection: O(W × H)
  - Voting: O(numEdges × numAngles × numRadii)
  - NMS: O(W × H × 9)
  - Duplicate removal: O(numCircles²)

### Memory Usage

- **Hough Lines**
  - Accumulator: O(numRho × numTheta) ≈ 1°resolution image
  - SinCosTable: O(numTheta)

- **Hough Circles**
  - Accumulator: O(W × H) [single accumulator, not per-radius]
  - Intermediate results: O(numEdges)

---

## Common Issues and Solutions

### Issue: Too Many False Positives

**Solution:** Increase threshold values
```cpp
params.threshold = 150;           // For lines
params.minAbsVotes = 30;          // For circles
params.cannyThreshold1 = 100.0;   // More strict edge detection
```

### Issue: Missing Faint Shapes

**Solution:** Decrease thresholds and improve edge detection
```cpp
params.threshold = 40;
params.cannyThreshold1 = 30.0;
params.minAbsVotes = 10;
params.param2 = 10.0;
```

### Issue: Duplicate Lines/Circles Detected

**Solution:** Increase tolerances for duplicate removal
```cpp
params.lineAngleTolerance = 2.0;      // Was 1.0
params.lineDistTolerance = 5.0;       // Was 2.0
params.circleRadiusTolerance = 0.50;  // Was 0.30
```

### Issue: Border Artifacts

**Solution:** Increase border margin
```cpp
params.borderMargin = 15;  // Was 5
```

### Issue: Slow Performance

**Solution:** Reduce resolution and sampling density
```cpp
params.rho = 2.0;                 // Coarser rho steps
params.theta = CV_PI / 90.0;      // Coarser theta steps (2°)
params.angleSampleDeg = 4.0;      // For circles
params.nmsWindowSize = 3;         // Smaller NMS
```

---

## Integration with ClarityCV API

The advanced Hough processor integrates seamlessly with the ClarityCV REST API:

```cpp
// In your API handler:
cv::Mat image = loadImageFromRequest(request);
HoughProcessor processor;
HoughParams params = parseParamsFromRequest(request);

HoughResult result = processor.apply(image, params);

// Return results
json response;
response["lines"] = convertToJSON(result.lines);
response["circles"] = convertToJSON(result.circles);
response["image"] = encodeImage(result.transformImage);
```

---

## Technical Details

### Theta Wrapping in NMS

The implementation correctly handles theta wrapping because theta is in [0, π):
```cpp
int tt = (t + dt + numTheta) % numTheta;  // Circular wrapping
```

This ensures that angles near 0 and near π are correctly compared as neighbors.

### Expected Votes Formula for Circles

For a circle of radius r, with angle sampling every ϕ degrees:
- Number of samples: N = 360° / ϕ
- Expected edge pixels on circumference: E = 2πr / (pixel_circumference_sampling)
- Threshold: T = max(minAbsVotes, param2 × E)

This makes the detector scale-aware: larger circles naturally receive more votes.

### NMS and Duplicate Removal

- **NMS:** Removes weak local maxima in (rho, theta) space
- **Duplicate Removal:** Removes near-identical detections in geometric space
- Both stages are necessary for robust, non-redundant output

---

## References

- Hough, P. V. (1962). Method and means for recognizing complex patterns
- Duda, R. O., & Hart, P. E. (1972). Use of the Hough transformation to detect lines and curves in pictures
- Yuen, H. K., et al. (1990). A comparative study of Hough transform methods for circle detection
