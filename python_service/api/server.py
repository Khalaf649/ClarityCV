"""FastAPI server for the Python backend (face recognition + segmentation).

Default port is 8081 so it doesn't clash with the C++ backend on 8080.
The frontend's Next.js rewrites route ``/api/recognize_faces`` and
``/api/segment`` here; everything else continues to the C++ backend.

Run locally::

    cd python_service
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
from fastapi import FastAPI, File, Form, HTTPException, UploadFile
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from PIL import Image

from core.pipeline import FaceRecognitionPipeline
from core.recognizer import EigenfaceRecognizer

from .schemas import (
    HealthResponse,
    MethodInfo,
    MethodsResponse,
    RecognizeRequest,
    RecognizeResponse,
    SegmentResponse,
)
from .segment_service import (
    ALLOWED_SEGMENTATION_METHODS,
    ALLOWED_THRESHOLD_METHODS,
    array_to_png_base64,
    read_upload_to_array,
    run_segmentation,
    run_thresholding,
)

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


# ---------------------------------------------------------------------------
# Segmentation / thresholding endpoints (merged from fastapi_image_segmenter)
#
# These come from a separate Python service the team built earlier (Project 4).
# Rather than running two Python services in Docker, we mount the routes here
# under the same FastAPI app — so the same container handles both face
# recognition and segmentation, on the same port (8081).
# ---------------------------------------------------------------------------


@app.get("/api/methods", response_model=MethodsResponse)
def methods() -> MethodsResponse:
    """List every available segmentation / thresholding method and its
    tunable parameters. The frontend calls this on page load to build
    parameter forms dynamically."""
    return MethodsResponse(
        thresholding=[
            MethodInfo(
                name="optimal",
                category="thresholding",
                accepts="grayscale or color; converted to grayscale internally",
                description="Iterative optimal global thresholding initialized from image corners.",
                params={"epsilon": 0.5, "max_iter": 100},
            ),
            MethodInfo(
                name="otsu",
                category="thresholding",
                accepts="grayscale or color; converted to grayscale internally",
                description="Classic Otsu thresholding from a 256-bin histogram.",
                params={},
            ),
            MethodInfo(
                name="spectral",
                category="thresholding",
                accepts="grayscale or color; converted to grayscale internally",
                description="Multi-level Otsu/spectral thresholding for more than two intensity modes.",
                params={"levels": 3},
            ),
            MethodInfo(
                name="local",
                category="thresholding",
                accepts="grayscale or color; converted to grayscale internally",
                description="Adaptive (mean-minus-offset) local thresholding inside a sliding window.",
                params={"window_size": 31, "offset": 5.0},
            ),
        ],
        segmentation=[
            MethodInfo(
                name="kmeans",
                category="segmentation",
                accepts="color (RGB)",
                description="K-means clustering in feature space (color, optionally with xy).",
                params={"k": 4, "include_xy": False, "max_iter": 40, "seed": 0},
            ),
            MethodInfo(
                name="region_growing",
                category="segmentation",
                accepts="grayscale or color",
                description="Flood-fill region growing from a seed point with intensity tolerance.",
                params={"seed_x": None, "seed_y": None, "threshold": 12.0, "connectivity": 8},
            ),
            MethodInfo(
                name="agglomerative",
                category="segmentation",
                accepts="color (RGB)",
                description="Hierarchical (bottom-up) agglomerative clustering on a sub-sampled palette.",
                params={"clusters": 4, "sample_size": 32},
            ),
            MethodInfo(
                name="mean_shift",
                category="segmentation",
                accepts="color (RGB)",
                description="Mean-shift clustering on a sub-sampled palette with optional merge radius.",
                params={"bandwidth": 0.18, "max_iter": 6, "sample_size": 48, "merge_radius": None},
            ),
        ],
    )


@app.post("/api/segment", response_model=SegmentResponse)
async def segment(
    file: UploadFile = File(...),
    category: str = Form(..., description="thresholding or segmentation"),
    method: str = Form(...),
    # thresholding params
    epsilon: Optional[float] = Form(None),
    max_iter: Optional[int] = Form(None),
    levels: Optional[int] = Form(None),
    window_size: Optional[int] = Form(None),
    offset: Optional[float] = Form(None),
    # segmentation params
    k: Optional[int] = Form(None),
    seed: Optional[int] = Form(None),
    include_xy: Optional[bool] = Form(None),
    seed_x: Optional[int] = Form(None),
    seed_y: Optional[int] = Form(None),
    threshold: Optional[float] = Form(None),
    connectivity: Optional[int] = Form(None),
    clusters: Optional[int] = Form(None),
    sample_size: Optional[int] = Form(None),
    bandwidth: Optional[float] = Form(None),
    merge_radius: Optional[float] = Form(None),
) -> SegmentResponse:
    """Run a thresholding / segmentation algorithm and return the result as a
    base64-PNG plus method-specific metadata.

    The endpoint accepts ``multipart/form-data`` because the original frontend
    code already sends files this way (and it's lighter than base64-encoding a
    full image when the user has a real File object in hand).
    """
    category = category.strip().lower()
    method = method.strip().lower()

    if not file.content_type or not file.content_type.startswith("image/"):
        raise HTTPException(status_code=400, detail="Please upload an image file.")

    params = {
        "epsilon": epsilon,
        "max_iter": max_iter,
        "levels": levels,
        "window_size": window_size,
        "offset": offset,
        "k": k,
        "seed": seed,
        "include_xy": include_xy,
        "seed_x": seed_x,
        "seed_y": seed_y,
        "threshold": threshold,
        "connectivity": connectivity,
        "clusters": clusters,
        "sample_size": sample_size,
        "bandwidth": bandwidth,
        "merge_radius": merge_radius,
    }
    params = {key: value for key, value in params.items() if value is not None}

    try:
        image = read_upload_to_array(await file.read(), mode="RGB")
    except Exception as exc:  # noqa: BLE001
        raise HTTPException(status_code=400, detail=f"Could not decode image: {exc}")

    try:
        if category == "thresholding":
            if method not in ALLOWED_THRESHOLD_METHODS:
                raise HTTPException(
                    status_code=400,
                    detail=f"Unknown thresholding method '{method}'. "
                           f"Allowed: {sorted(ALLOWED_THRESHOLD_METHODS)}",
                )
            output, metadata = run_thresholding(method, image, params)
        elif category == "segmentation":
            if method not in ALLOWED_SEGMENTATION_METHODS:
                raise HTTPException(
                    status_code=400,
                    detail=f"Unknown segmentation method '{method}'. "
                           f"Allowed: {sorted(ALLOWED_SEGMENTATION_METHODS)}",
                )
            output, metadata = run_segmentation(method, image, params)
        else:
            raise HTTPException(
                status_code=400,
                detail=f"Unknown category '{category}'. Use 'thresholding' or 'segmentation'.",
            )
    except HTTPException:
        raise
    except ValueError as exc:
        raise HTTPException(status_code=400, detail=str(exc))
    except Exception as exc:  # noqa: BLE001
        log.exception("segmentation failed")
        raise HTTPException(status_code=500, detail=f"Internal error: {exc}")

    log.info("segment: category=%s method=%s ok", category, method)

    return SegmentResponse(
        method=method,
        filename=file.filename or "image.png",
        mime_type="image/png",
        image_base64=array_to_png_base64(output),
        metadata=metadata,
    )


# Convenience for `python -m api.server`
if __name__ == "__main__":
    import uvicorn

    port = int(os.environ.get("PORT", 8081))
    uvicorn.run("api.server:app", host="0.0.0.0", port=port, reload=False)
