import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  async rewrites() {
    const backendUrl = process.env.BACKEND_URL ?? "http://localhost:8080";
    const faceRecognitionUrl =
      process.env.FACE_RECOGNITION_URL ?? "http://localhost:8081";

    return [
      // Face recognition is served by a separate Python/FastAPI service.
      // This rule must come before the catch-all so it wins.
      {
        source: "/api/recognize_faces",
        destination: `${faceRecognitionUrl}/api/recognize_faces`,
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
