import {
  Activity,
  BarChart2,
  Crosshair,
  Image as ImageIcon,
  Layers,
  Link as LinkIcon,
  Users,
  Zap,
} from "lucide-react";

export const appGroups = [
  {
    category: "Image Enhancement",
    tools: [
      {
        href: "/spatial",
        title: "Spatial Filters",
        description:
          "Apply noise models (uniform, Gaussian, salt & pepper) and spatial-domain filters including average, Gaussian, median, and edge detectors (Sobel, Roberts, Prewitt, Canny).",
        icon: <ImageIcon size={20} />,
      },
      {
        href: "/histogram",
        title: "Histogram & Equalization",
        description:
          "Visualise per-channel histograms, convert to grayscale, normalise intensity ranges, and perform histogram equalisation.",
        icon: <BarChart2 size={20} />,
      },
      {
        href: "/frequency-domain",
        title: "Frequency Domain",
        description:
          "Explore FFT-based low/high-pass filtering with real-time spectrum preview, and create hybrid images by mixing frequency components of two images.",
        icon: <Activity size={20} />,
      },
    ],
  },
  {
    category: "Feature Detection",
    tools: [
      {
        href: "/active_contour",
        title: "Active Contour",
        description:
          "Use active contours (snakes) to detect and trace object boundaries. Place control points on an image and configure alpha, beta, and gamma parameters to guide the contour evolution.",
        icon: <Layers size={20} />,
      },
      {
        href: "/corner-detection",
        title: "Corner Detection",
        description:
          "Detect corners in images using the Harris or Shi-Tomasi (λ-) methods. Interactively adjust the detection mode and parameters like k-factor.",
        icon: <Crosshair size={20} />,
      },
      {
        href: "/sift",
        title: "SIFT Keypoints",
        description:
          "Detect and visualize SIFT keypoints. Adjust contrast threshold and number of features for the detector.",
        icon: <Crosshair size={20} />,
      },
      {
        href: "/feature-matching",
        title: "Feature Matching",
        description:
          "Match features between two images using SSD or NCC. Upload two images or reuse the global image as Image 1.",
        icon: <LinkIcon size={20} />,
      },
    ],
  },
  {
    category: "Segmentation",
    tools: [
      {
        href: "/segmentaion/thresholding",
        title: "Thresholding",
        description:
          "Apply thresholding methods including Optimal (iterative), Otsu, Spectral (multi-level), and Local (adaptive). Convert images into binary masks based on intensity.",
        icon: <Zap size={20} />,
      },
      {
        href: "/segmentaion/advanced",
        title: "Advanced Segmentation",
        description:
          "Apply advanced segmentation techniques including K-means clustering, region growing, agglomerative clustering, and mean shift.",
        icon: <Zap size={20} />,
      },
    ],
  },
  {
    category: "Face Detection & Recognition",
    tools: [
      {
        href: "/face-recognition",
        title: "Face Recognition",
        description:
          "Detect and recognize faces in images using state-of-the-art face recognition algorithms. Adjust threshold parameters for better accuracy.",
        icon: <Users size={20} />,
      },
    ],
  },
];
