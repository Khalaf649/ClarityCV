import Link from "next/link";

const routes = [
  {
    href: "/spatial",
    title: "Spatial Filters",
    description:
      "Apply noise models (uniform, Gaussian, salt & pepper) and spatial-domain filters including average, Gaussian, median, and edge detectors (Sobel, Roberts, Prewitt, Canny).",
    icon: "🔲",
  },
  {
    href: "/histogram",
    title: "Histogram & Equalization",
    description:
      "Visualise per-channel histograms, convert to grayscale, normalise intensity ranges, and perform histogram equalisation.",
    icon: "📊",
  },
  {
    href: "/frequency-domain",
    title: "Frequency Domain",
    description:
      "Explore FFT-based low/high-pass filtering with real-time spectrum preview, and create hybrid images by mixing frequency components of two images.",
    icon: "🌊",
  },
  {
    href: "/active_contour",
    title: "Active Contour",
    description:
      "Use active contours (snakes) to detect and trace object boundaries. Place control points on an image and configure alpha, beta, and gamma parameters to guide the contour evolution.",
    icon: "🐍",
  },
  {
    href: "/corner-detection",
    title: "Corner Detection",
    description:
      "Detect corners in images using the Harris or Shi-Tomasi (λ-) methods. Interactively adjust the detection mode and parameters like k-factor.",
    icon: "📐",
  },
  {
    href: "/sift",
    title: "SIFT Keypoints",
    description:
      "Detect and visualize SIFT keypoints. Adjust contrast threshold and number of features for the detector.",
    icon: "🔍",
  },
];

export default function HomePage() {
  return (
    <main className="flex-1 flex flex-col items-center justify-center p-6 overflow-auto">
      {/* Hero */}
      <section className="max-w-3xl text-center mb-12">
        <h1 className="text-4xl md:text-5xl font-bold tracking-tight mb-4">
          <span className="text-primary">CV</span>Lab
        </h1>
        <p className="text-lg text-muted-foreground leading-relaxed">
          An interactive computer-vision laboratory. Upload an image once and
          explore spatial filtering, histogram analysis, and frequency-domain
          operations — all from your browser.
        </p>
      </section>

      {/* Route cards */}
      <section className="grid grid-cols-1 md:grid-cols-3 gap-6 w-full max-w-5xl">
        {routes.map((route) => (
          <Link
            key={route.href}
            href={route.href}
            className="group control-panel glow-border hover:border-primary transition-all duration-200 no-underline"
          >
            <span className="text-3xl mb-2 block">{route.icon}</span>
            <h2 className="text-base font-mono font-semibold text-primary group-hover:underline">
              {route.title}
            </h2>
            <p className="text-sm text-muted-foreground mt-1 leading-relaxed">
              {route.description}
            </p>
          </Link>
        ))}
      </section>
    </main>
  );
}
