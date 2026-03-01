// ---------------------------------------------------------------------------
// Canvas & file helpers only — all image processing is done by the backend.
// ---------------------------------------------------------------------------

/** Read a File and return its base64-encoded content (without data URI prefix). */
export async function fileToBase64(file: File): Promise<string> {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => {
      const result = reader.result as string;
      const base64 = result.split(",")[1];
      resolve(base64);
    };
    reader.onerror = reject;
    reader.readAsDataURL(file);
  });
}

/** Build a displayable data URI from a raw base64 string returned by the backend. */
export function toDataUrl(base64: string, mime = "image/png"): string {
  if (base64.startsWith("data:")) return base64;
  return `data:${mime};base64,${base64}`;
}
