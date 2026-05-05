import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  async rewrites() {
    const backendUrl = process.env.BACKEND_URL ?? "http://localhost:8080";
    // python_service hosts both face recognition and segmentation. Inside
    // Docker compose this resolves to the service hostname; in local dev,
    // the env var is unset and we fall back to localhost.
    const pythonServiceUrl =
      process.env.PYTHON_SERVICE_URL ?? "http://localhost:8081";

    return [
      // Routes handled by python_service (face recognition + segmentation).
      // These rules must come before the catch-all so they win — Next.js
      // matches rewrites top-to-bottom.
      {
        source: "/api/recognize_faces",
        destination: `${pythonServiceUrl}/api/recognize_faces`,
      },
      {
        source: "/api/segment",
        destination: `${pythonServiceUrl}/api/segment`,
      },
      {
        source: "/api/methods",
        destination: `${pythonServiceUrl}/api/methods`,
      },
      // Everything else goes to the C++ backend.
      {
        source: "/api/:path*",
        destination: `${backendUrl}/api/:path*`,
      },
    ];
  },
};

export default nextConfig;
