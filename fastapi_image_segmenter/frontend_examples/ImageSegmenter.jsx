import { useState } from "react";
import { segmentImage } from "./api";

export default function ImageSegmenter() {
  const [file, setFile] = useState(null);
  const [category, setCategory] = useState("thresholding");
  const [method, setMethod] = useState("otsu");
  const [resultUrl, setResultUrl] = useState("");
  const [metadata, setMetadata] = useState(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");

  const methodOptions =
    category === "thresholding"
      ? ["optimal", "otsu", "spectral", "local"]
      : ["kmeans", "region_growing", "agglomerative", "mean_shift"];

  async function handleSubmit(event) {
    event.preventDefault();
    if (!file) return;

    setLoading(true);
    setError("");
    setResultUrl("");
    setMetadata(null);

    try {
      const data = await segmentImage({
        file,
        category,
        method,
        params: {
          levels: method === "spectral" ? 3 : undefined,
          window_size: method === "local" ? 31 : undefined,
          offset: method === "local" ? 5 : undefined,
          k: method === "kmeans" ? 4 : undefined,
          clusters: method === "agglomerative" ? 4 : undefined,
          bandwidth: method === "mean_shift" ? 0.18 : undefined,
          threshold: method === "region_growing" ? 12 : undefined,
          connectivity: method === "region_growing" ? 8 : undefined,
        },
      });
      setResultUrl(data.imageUrl);
      setMetadata(data.metadata);
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  }

  return (
    <div style={{ maxWidth: 900, margin: "40px auto", fontFamily: "Arial, sans-serif" }}>
      <h1>No-OpenCV Image Segmenter</h1>

      <form onSubmit={handleSubmit} style={{ display: "grid", gap: 16 }}>
        <input type="file" accept="image/*" onChange={(e) => setFile(e.target.files?.[0] || null)} />

        <label>
          Category
          <select
            value={category}
            onChange={(e) => {
              const nextCategory = e.target.value;
              setCategory(nextCategory);
              setMethod(nextCategory === "thresholding" ? "otsu" : "kmeans");
            }}
          >
            <option value="thresholding">Thresholding</option>
            <option value="segmentation">Segmentation</option>
          </select>
        </label>

        <label>
          Method
          <select value={method} onChange={(e) => setMethod(e.target.value)}>
            {methodOptions.map((option) => (
              <option key={option} value={option}>
                {option}
              </option>
            ))}
          </select>
        </label>

        <button disabled={!file || loading}>{loading ? "Processing..." : "Run"}</button>
      </form>

      {error && <p style={{ color: "crimson" }}>{error}</p>}

      {resultUrl && (
        <div style={{ marginTop: 24 }}>
          <h2>Result</h2>
          <img src={resultUrl} alt="Segmentation result" style={{ maxWidth: "100%", border: "1px solid #ddd" }} />
          <pre>{JSON.stringify(metadata, null, 2)}</pre>
        </div>
      )}
    </div>
  );
}
