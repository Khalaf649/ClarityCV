"""FastAPI server for the face-recognition feature.

Default port is 8081 so it doesn't clash with the C++ backend on 8080.
The frontend's Next.js rewrite already routes ``/api/recognize_faces`` here
while everything else continues to the C++ backend.

Run locally::

    cd face_recognition_service
    python -m api.server                    # uses MODEL_PATH env var

Run with uvicorn::

    uvicorn api.server:app --host 0.0.0.0 --port 8081 --reload

Environment variables:

* ``MODEL_PATH`` — path to ``trained_model.pkl`` (default ``./trained_model.pkl``)
* ``PORT``       — listen port (default ``8081``)
"""
from __future__ import annotations

import base64
import io
import logging
import os
import time
from pathlib import Path
from typing import Optional

import cv2
import numpy as np
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from PIL import Image

from core.pipeline import FaceRecognitionPipeline
from core.recognizer import EigenfaceRecognizer

from .schemas import HealthResponse, RecognizeRequest, RecognizeResponse

# ---------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
log = logging.getLogger("face_recognition")

# ---------------------------------------------------------------------------
# Pipeline (lazy-loaded so import doesn't fail when the model is missing)
# ---------------------------------------------------------------------------

_pipeline: Optional[FaceRecognitionPipeline] = None
_model_load_error: Optional[str] = None


def get_pipeline() -> FaceRecognitionPipeline:
    global _pipeline, _model_load_error
    if _pipeline is not None:
        return _pipeline

    model_path = Path(os.environ.get("MODEL_PATH", "trained_model.pkl"))
    log.info("Loading model from %s", model_path.resolve())
    try:
        recognizer = EigenfaceRecognizer.from_pickle(model_path)
    except Exception as exc:  # noqa: BLE001
        _model_load_error = f"{type(exc).__name__}: {exc}"
        log.error("Failed to load model: %s", _model_load_error)
        raise HTTPException(
            status_code=503,
            detail=(
                f"Model not loaded. {_model_load_error}. "
                "Set MODEL_PATH or run tools/inspect_model.py to debug."
            ),
        )

    log.info(
        "Model loaded — %d components, %d training projections, %d unique labels, "
        "image_shape=%s",
        recognizer.n_components,
        recognizer.n_train,
        len(recognizer.unique_labels),
        recognizer.image_shape,
    )
    _pipeline = FaceRecognitionPipeline(recognizer=recognizer)
    return _pipeline


# ---------------------------------------------------------------------------
# Image encoding helpers
# ---------------------------------------------------------------------------

# Cap incoming images to this max dimension on the longest side. Phone
# photos are routinely 12+ MP — running Haar over that takes 20+ seconds
# and there's no accuracy gain since faces only need ~100px to detect.
# 1500 keeps detection accurate while bounding worst-case latency.
MAX_INPUT_DIM = 1500


def _decode_base64_image(data: str) -> np.ndarray:
    """Accepts plain base64 or data URI. Returns BGR uint8 (OpenCV layout).
    Downscales (preserving aspect ratio) if the longest side exceeds
    ``MAX_INPUT_DIM`` so Haar detection stays fast on very large uploads."""
    if "," in data and data.lstrip().startswith("data:"):
        data = data.split(",", 1)[1]
    raw = base64.b64decode(data)
    pil = Image.open(io.BytesIO(raw))
    pil.load()
    if pil.mode not in ("RGB", "L"):
        pil = pil.convert("RGB")

    # Downscale on the PIL side — cheaper than going through numpy first.
    longest = max(pil.size)
    if longest > MAX_INPUT_DIM:
        scale = MAX_INPUT_DIM / longest
        new_size = (max(1, int(pil.size[0] * scale)), max(1, int(pil.size[1] * scale)))
        pil = pil.resize(new_size, Image.LANCZOS)
        log.info("Downscaled input from %d→%d px on longest side", longest, MAX_INPUT_DIM)

    arr = np.asarray(pil)
    if arr.ndim == 3:
        # PIL gives RGB, OpenCV uses BGR.
        arr = cv2.cvtColor(arr, cv2.COLOR_RGB2BGR)
    return arr


def _encode_png_base64(image_bgr: np.ndarray) -> str:
    if image_bgr.ndim == 2:
        rgb = cv2.cvtColor(image_bgr, cv2.COLOR_GRAY2RGB)
    else:
        rgb = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2RGB)
    pil = Image.fromarray(rgb)
    buf = io.BytesIO()
    pil.save(buf, format="PNG")
    return base64.b64encode(buf.getvalue()).decode("ascii")


# ---------------------------------------------------------------------------
# App
# ---------------------------------------------------------------------------

app = FastAPI(title="ClarityCV Face Recognition", version="1.1.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.get("/api/health")
def health() -> JSONResponse:
    """Returns 200 only when the recognition model is fully loaded.
    503 otherwise — this is what the Docker healthcheck (and the frontend's
    ``depends_on: service_healthy``) rely on."""
    try:
        pipe = get_pipeline()
        body = HealthResponse(
            status="ok",
            model_loaded=True,
            n_components=pipe.recognizer.n_components,
            n_train=pipe.recognizer.n_train,
            n_classes=len(pipe.recognizer.unique_labels),
            image_shape=list(pipe.recognizer.image_shape),
        )
        return JSONResponse(content=body.model_dump(), status_code=200)
    except HTTPException as exc:
        body = HealthResponse(
            status="error",
            model_loaded=False,
            error=str(exc.detail),
        )
        return JSONResponse(content=body.model_dump(), status_code=503)


@app.post("/api/recognize_faces", response_model=RecognizeResponse)
def recognize_faces(req: RecognizeRequest) -> RecognizeResponse:
    pipeline = get_pipeline()

    try:
        image_bgr = _decode_base64_image(req.image)
    except Exception as exc:  # noqa: BLE001
        raise HTTPException(status_code=400, detail=f"Invalid image data: {exc}")

    # The frontend's dropdown lets users pick "face_recognition" (full PCA
    # identification) or "haar_detection" (boxes only, no labels).
    method = (req.method or "face_recognition").lower()
    detect_only = method in {"haar_detection", "detection", "detect"}

    t0 = time.perf_counter()
    annotated, results, _ = pipeline.run(
        image_bgr,
        threshold=req.threshold,
        detect_only=detect_only,
    )
    total = time.perf_counter() - t0

    # Faces "detected" in the UI sense:
    #   - recognition mode: count matched (non-Unknown) identities
    #   - detection mode:   count every face the cascade found
    if detect_only:
        face_count = len(results)
    else:
        face_count = sum(1 for r in results if r.recognition.is_match)

    log.info(
        "recognize: method=%s, %d face(s) found, %d reported, %.3fs",
        method,
        len(results),
        face_count,
        total,
    )

    return RecognizeResponse(
        success=True,
        image=_encode_png_base64(annotated),
        facesDetected=face_count,
        computationTime=total,
    )


# Convenience for `python -m api.server`
if __name__ == "__main__":
    import uvicorn

    port = int(os.environ.get("PORT", 8081))
    uvicorn.run("api.server:app", host="0.0.0.0", port=port, reload=False)
