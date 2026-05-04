// Put this file in your React project, for example: src/api/imageSegmentationApi.js

const API_BASE_URL = import.meta.env.VITE_API_BASE_URL || "http://localhost:8000";

export async function segmentImage({ file, category, method, params = {} }) {
  const formData = new FormData();
  formData.append("file", file);
  formData.append("category", category); // "thresholding" or "segmentation"
  formData.append("method", method);     // "otsu", "kmeans", etc.

  Object.entries(params).forEach(([key, value]) => {
    if (value !== undefined && value !== null && value !== "") {
      formData.append(key, value);
    }
  });

  const response = await fetch(`${API_BASE_URL}/api/segment`, {
    method: "POST",
    body: formData,
  });

  if (!response.ok) {
    const error = await response.json().catch(() => ({}));
    throw new Error(error.detail || "Segmentation request failed");
  }

  const data = await response.json();
  return {
    ...data,
    imageUrl: `data:${data.mime_type};base64,${data.image_base64}`,
  };
}

export async function getMethods() {
  const response = await fetch(`${API_BASE_URL}/api/methods`);
  if (!response.ok) throw new Error("Could not load methods");
  return response.json();
}
