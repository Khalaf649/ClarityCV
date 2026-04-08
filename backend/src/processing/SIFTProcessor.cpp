#define _USE_MATH_DEFINES

#include "processing/SIFTProcessor.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>

namespace processing {

// ==============================================================================
// STEP 1: Build Scale-Space (Gaussian Pyramid)
// ==============================================================================
std::vector<std::vector<cv::Mat>> SIFTProcessor::stage_buildScaleSpace(
    const cv::Mat& gray, 
    const SIFTParams& params) 
{
    std::vector<std::vector<cv::Mat>> pyramid;
    
    // Build Gaussian pyramid with multiple octaves
    for (int octave = 0; octave < params.octaves; ++octave) {
        std::vector<cv::Mat> octaveLevels;
        cv::Mat base = gray.clone();
        
        // Downsample for each octave
        for (int o = 0; o < octave; ++o) {
            cv::pyrDown(base, base);
        }
        
        // Build Gaussian blur sequence for this octave
        double sigma = params.sigma;
        for (int scale = 0; scale < params.scales + 3; ++scale) {
            double nextSigma = params.sigma * std::pow(2.0, octave + scale / (double)params.scales);
            double blurSigma = std::sqrt(nextSigma * nextSigma - sigma * sigma);
            sigma = nextSigma;
            
            cv::Mat blurred = base.clone();
            if (blurSigma > 0.5) {
                int ksize = static_cast<int>(6.0 * blurSigma + 1.0) | 1;
                cv::GaussianBlur(base, blurred, cv::Size(ksize, ksize), blurSigma);
            }
            octaveLevels.push_back(blurred);
        }
        
        pyramid.push_back(octaveLevels);
    }
    
    return pyramid;
}

// ==============================================================================
// STEP 2: Compute Difference of Gaussian (DoG)
// ==============================================================================
std::vector<std::vector<cv::Mat>> SIFTProcessor::stage_buildDoGPyramid(
    const std::vector<std::vector<cv::Mat>>& gaussianPyramid)
{
    std::vector<std::vector<cv::Mat>> dogPyramid;
    
    // Subtract adjacent Gaussian blurs
    for (const auto& octave : gaussianPyramid) {
        std::vector<cv::Mat> dogOctave;
        
        for (size_t i = 0; i + 1 < octave.size(); ++i) {
            cv::Mat dog;
            cv::subtract(octave[i + 1], octave[i], dog);
            dogOctave.push_back(dog);
        }
        
        dogPyramid.push_back(dogOctave);
    }
    
    return dogPyramid;
}

// ==============================================================================
// STEP 3: Detect Keypoints in DoG (Local Extrema)
// ==============================================================================
std::vector<cv::KeyPoint> SIFTProcessor::stage_detectKeypointExtrema(
    const std::vector<std::vector<cv::Mat>>& dogPyramid,
    const SIFTParams& params)
{
    std::vector<cv::KeyPoint> keypoints;
    
    // Check each pixel in each scale in each octave
    for (size_t octave = 0; octave < dogPyramid.size(); ++octave) {
        const auto& dogOctave = dogPyramid[octave];
        
        // Examine scales 1 to n-2 (skip first and last)
        for (size_t scale = 1; scale + 1 < dogOctave.size(); ++scale) {
            const cv::Mat& prevScale = dogOctave[scale - 1];
            const cv::Mat& currScale = dogOctave[scale];
            const cv::Mat& nextScale = dogOctave[scale + 1];
            
            // Check each pixel (skip borders)
            for (int y = 1; y < currScale.rows - 1; ++y) {
                for (int x = 1; x < currScale.cols - 1; ++x) {
                    float center = currScale.at<float>(y, x);
                    
                    // Check if center is local max or min in 3×3×3 neighborhood
                    bool isLocalMax = true;
                    bool isLocalMin = true;
                    
                    for (int dy = -1; dy <= 1 && (isLocalMax || isLocalMin); ++dy) {
                        for (int dx = -1; dx <= 1 && (isLocalMax || isLocalMin); ++dx) {
                            for (int ds = -1; ds <= 1 && (isLocalMax || isLocalMin); ++ds) {
                                if (dy == 0 && dx == 0 && ds == 0) continue;
                                
                                const cv::Mat& checkScale = (ds < 0) ? prevScale : 
                                                           (ds > 0) ? nextScale : currScale;
                                float val = checkScale.at<float>(y + dy, x + dx);
                                
                                if (center <= val) isLocalMax = false;
                                if (center >= val) isLocalMin = false;
                            }
                        }
                    }
                    
                    // Found a local extremum with sufficient contrast
                    if ((isLocalMax || isLocalMin) && std::abs(center) > params.peakThreshold) {
                        // Scale coordinates back to original image space
                        float scaledX = x * std::pow(2.0f, octave);
                        float scaledY = y * std::pow(2.0f, octave);
                        
                        // Set size proportional to scale
                        float size = params.sigma * std::pow(2.0f, octave + scale / (float)params.scales);
                        size = std::max(size, 5.0f);  // Minimum size for visibility
                        
                        cv::KeyPoint kp(scaledX, scaledY, size);
                        kp.octave = octave;
                        kp.class_id = scale;
                        keypoints.push_back(kp);
                    }
                }
            }
        }
    }
    
    return keypoints;
}

// ==============================================================================
// STEP 4: Keypoint Localization (Filter & Refine)
// ==============================================================================
std::vector<cv::KeyPoint> SIFTProcessor::stage_localizeAndFilterKeypoints(
    const std::vector<cv::KeyPoint>& rawKeypoints,
    const std::vector<std::vector<cv::Mat>>& dogPyramid,
    const std::vector<std::vector<cv::Mat>>& gaussianPyramid,
    const SIFTParams& params)
{
    std::vector<cv::KeyPoint> filtered;
    
    for (auto kp : rawKeypoints) {
        int octave = kp.octave;
        int scale = kp.class_id;
        
        if (octave < 0 || octave >= static_cast<int>(gaussianPyramid.size())) continue;
        if (scale < 1 || scale + 1 >= static_cast<int>(gaussianPyramid[octave].size())) continue;
        
        // Convert to octave-space coordinates
        float octaveX = kp.pt.x / std::pow(2.0f, octave);
        float octaveY = kp.pt.y / std::pow(2.0f, octave);
        
        int x = static_cast<int>(octaveX);
        int y = static_cast<int>(octaveY);
        
        const cv::Mat& gauss = gaussianPyramid[octave][scale];
        
        if (x > 0 && x < gauss.cols - 1 && y > 0 && y < gauss.rows - 1) {
            // Compute Hessian
            float Lxx = gauss.at<float>(y, x + 1) + gauss.at<float>(y, x - 1) 
                      - 2.0f * gauss.at<float>(y, x);
            float Lyy = gauss.at<float>(y + 1, x) + gauss.at<float>(y - 1, x) 
                      - 2.0f * gauss.at<float>(y, x);
            float Lxy = (gauss.at<float>(y + 1, x + 1) - gauss.at<float>(y + 1, x - 1) 
                       - gauss.at<float>(y - 1, x + 1) + gauss.at<float>(y - 1, x - 1)) / 4.0f;
            
            float trace = Lxx + Lyy;
            float deter = Lxx * Lyy - Lxy * Lxy;
            
            // Remove edge responses: trace²/determinant ratio
            if (deter > 1e-6) {
                float ratio = (trace * trace) / deter;
                float threshold = (params.edgeThreshold + 1.0f) * (params.edgeThreshold + 1.0f) / params.edgeThreshold;
                
                if (ratio < threshold) {
                    filtered.push_back(kp);
                }
            }
        }
    }
    
    return filtered;
}

// ==============================================================================
// STEP 5: Assign Orientation to Keypoints
// ==============================================================================
void SIFTProcessor::stage_assignOrientations(
    std::vector<cv::KeyPoint>& keypoints,
    const std::vector<std::vector<cv::Mat>>& gaussianPyramid,
    const SIFTParams& params)
{
    const int ORIENTATION_BINS = 36;
    
    for (auto& kp : keypoints) {
        int octave = kp.octave;
        int scale = kp.class_id;
        
        if (octave < 0 || octave >= static_cast<int>(gaussianPyramid.size())) continue;
        if (scale < 0 || scale >= static_cast<int>(gaussianPyramid[octave].size())) continue;
        
        const cv::Mat& gauss = gaussianPyramid[octave][scale];
        
        // Convert to octave-space coordinates
        float octaveX = kp.pt.x / std::pow(2.0f, octave);
        float octaveY = kp.pt.y / std::pow(2.0f, octave);
        
        int cx = static_cast<int>(octaveX);
        int cy = static_cast<int>(octaveY);
        
        // Build orientation histogram
        std::vector<float> histogram(ORIENTATION_BINS, 0);
        int radius = std::max(3, static_cast<int>(kp.size / 2.0f));
        
        // Compute gradients in local neighborhood
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                int y = cy + dy;
                int x = cx + dx;
                
                if (x > 0 && x < gauss.cols - 1 && y > 0 && y < gauss.rows - 1) {
                    float gx = gauss.at<float>(y, x + 1) - gauss.at<float>(y, x - 1);
                    float gy = gauss.at<float>(y + 1, x) - gauss.at<float>(y - 1, x);
                    
                    float angle = std::atan2(gy, gx);
                    float magnitude = std::sqrt(gx * gx + gy * gy);
                    
                    // Gaussian weighting
                    float weight = std::exp(-(dx * dx + dy * dy) / (2.0f * radius * radius));
                    
                    // Map angle to histogram bin
                    int bin = static_cast<int>((angle + M_PI) / (2.0f * M_PI) * ORIENTATION_BINS);
                    bin = (bin + ORIENTATION_BINS) % ORIENTATION_BINS;
                    
                    histogram[bin] += magnitude * weight;
                }
            }
        }
        
        // Find dominant orientation
        auto maxIt = std::max_element(histogram.begin(), histogram.end());
        if (maxIt != histogram.end() && *maxIt > 0) {
            int maxBin = std::distance(histogram.begin(), maxIt);
            float angle = (maxBin * 2.0f * M_PI / ORIENTATION_BINS) - M_PI;
            kp.angle = angle * 180.0f / M_PI;
        }
    }
}

// ==============================================================================
// STEP 6: Compute Descriptor (128-D vector)
// ==============================================================================
cv::Mat SIFTProcessor::stage_generateDescriptors(
    const std::vector<cv::KeyPoint>& keypoints,
    const std::vector<std::vector<cv::Mat>>& gaussianPyramid,
    const SIFTParams& params)
{
    const int DESCRIPTOR_SIZE = 128;
    const int GRID_SIZE = 4;           // 4×4 spatial grid
    const int ANGLE_BINS = 8;          // 8 orientation bins per block
    
    cv::Mat descriptors(keypoints.size(), DESCRIPTOR_SIZE, CV_32F, cv::Scalar(0));
    
    for (size_t kpIdx = 0; kpIdx < keypoints.size(); ++kpIdx) {
        const auto& kp = keypoints[kpIdx];
        int octave = kp.octave;
        int scale = kp.class_id;
        
        if (octave < 0 || octave >= static_cast<int>(gaussianPyramid.size())) continue;
        if (scale < 0 || scale >= static_cast<int>(gaussianPyramid[octave].size())) continue;
        
        const cv::Mat& gauss = gaussianPyramid[octave][scale];
        
        // Convert to octave-space
        float octaveX = kp.pt.x / std::pow(2.0f, octave);
        float octaveY = kp.pt.y / std::pow(2.0f, octave);
        
        float angle = kp.angle * M_PI / 180.0f;
        std::vector<float> descriptor(DESCRIPTOR_SIZE, 0);
        
        // Extract 16×16 neighborhood → divide into 4×4 blocks
        int descIdx = 0;
        int regionSize = 2;  // Each region is 2×2 pixels
        
        for (int gy = 0; gy < GRID_SIZE; ++gy) {
            for (int gx = 0; gx < GRID_SIZE; ++gx) {
                // Sample points in this grid cell
                for (int dy = 0; dy < regionSize; ++dy) {
                    for (int dx = 0; dx < regionSize; ++dx) {
                        // Compute position relative to keypoint
                        float px = octaveX + (gx - 1.5f) * regionSize * 2 + dx * 2;
                        float py = octaveY + (gy - 1.5f) * regionSize * 2 + dy * 2;
                        
                        int x = static_cast<int>(px);
                        int y = static_cast<int>(py);
                        
                        if (x > 0 && x < gauss.cols - 1 && y > 0 && y < gauss.rows - 1) {
                            // Compute gradient
                            float gradX = gauss.at<float>(y, x + 1) - gauss.at<float>(y, x - 1);
                            float gradY = gauss.at<float>(y + 1, x) - gauss.at<float>(y - 1, x);
                            
                            float magnitude = std::sqrt(gradX * gradX + gradY * gradY);
                            float gradAngle = std::atan2(gradY, gradX) - angle;
                            
                            // Map to bin (0-7)
                            int bin = static_cast<int>((gradAngle + M_PI) / (2.0f * M_PI) * ANGLE_BINS);
                            bin = (bin + ANGLE_BINS) % ANGLE_BINS;
                            
                            descriptor[gy * GRID_SIZE * ANGLE_BINS + gx * ANGLE_BINS + bin] += magnitude;
                        }
                    }
                }
            }
        }
        
        // Normalize descriptor
        float norm = 0;
        for (float val : descriptor) norm += val * val;
        norm = std::sqrt(norm);
        
        if (norm > 1e-6) {
            for (int i = 0; i < DESCRIPTOR_SIZE; ++i) {
                descriptors.at<float>(kpIdx, i) = descriptor[i] / norm;
            }
        }
    }
    
    return descriptors;
}

// ==============================================================================
// MAIN: Apply SIFT from scratch
// ==============================================================================
SIFTResult SIFTProcessor::apply(const cv::Mat& input, const SIFTParams& params) {
    using Clock = std::chrono::high_resolution_clock;
    auto start = Clock::now();

    SIFTResult result;
    if (input.empty()) {
        result.image = cv::Mat();
        result.featureCount = 0;
        result.computationTimeMs = 0;
        return result;
    }

    // STEP 1: Convert to grayscale
    cv::Mat gray;
    if (input.channels() == 3) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else if (input.channels() == 4) {
        cv::cvtColor(input, gray, cv::COLOR_BGRA2GRAY);
    } else {
        gray = input.clone();
    }
    gray.convertTo(gray, CV_32F, 1.0f / 255.0f);

    // STEP 2-6: Build scale-space → DoG → Detect → Localize → Orient → Describe
    auto gaussPyramid = stage_buildScaleSpace(gray, params);
    auto dogPyramid = stage_buildDoGPyramid(gaussPyramid);
    auto keypoints = stage_detectKeypointExtrema(dogPyramid, params);
    keypoints = stage_localizeAndFilterKeypoints(keypoints, dogPyramid, gaussPyramid, params);
    
    // Limit keypoints to nfeatures (sort by size/strength)
    if (keypoints.size() > static_cast<size_t>(params.nfeatures)) {
        std::sort(keypoints.begin(), keypoints.end(), 
            [](const cv::KeyPoint& a, const cv::KeyPoint& b) { 
                return a.response > b.response;  // Sort by response (descriptor strength)
            });
        keypoints.resize(params.nfeatures);
    }
    
    stage_assignOrientations(keypoints, gaussPyramid, params);
    auto descriptors = stage_generateDescriptors(keypoints, gaussPyramid, params);

    result.keypoints = keypoints;
    result.descriptors = descriptors;
    result.featureCount = static_cast<int>(keypoints.size());

    // Visualize: Draw circles with orientation lines
    cv::Mat output = input.clone();
    
    if (params.drawKeypoints) {
        cv::Scalar circleColor(params.circleColor_B, params.circleColor_G, params.circleColor_R);
        cv::Scalar lineColor(params.lineColor_B, params.lineColor_G, params.lineColor_R);
        
        for (const auto& kp : keypoints) {
            cv::Point center(static_cast<int>(kp.pt.x), static_cast<int>(kp.pt.y));
            int radius = static_cast<int>(kp.size / 2.0f);
            
            // Draw outer circle
            cv::circle(output, center, radius, circleColor, params.circleThickness);
            
            // Draw center point (white)
            cv::circle(output, center, 3, cv::Scalar(255, 255, 255), -1);
            
            // Draw orientation line
            if (std::abs(kp.angle) > 1e-6) {
                float angle = kp.angle * M_PI / 180.0f;
                int lineLen = radius + 8;
                cv::Point endpoint(
                    center.x + static_cast<int>(lineLen * std::cos(angle)),
                    center.y + static_cast<int>(lineLen * std::sin(angle))
                );
                cv::arrowedLine(output, center, endpoint, lineColor, params.lineThickness);
            }
        }
    }
    
    result.image = output;

    auto end = Clock::now();
    result.computationTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    return result;
}

} // namespace processing
