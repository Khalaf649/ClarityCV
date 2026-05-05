from __future__ import annotations

from typing import Optional

from fastapi import FastAPI, File, Form, HTTPException, UploadFile
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import HTMLResponse

from app.image_service import (
    ALLOWED_SEGMENTATION_METHODS,
    ALLOWED_THRESHOLD_METHODS,
    array_to_png_base64,
    read_upload_to_array,
    run_segmentation,
    run_thresholding,
)
from app.schemas import MethodInfo, MethodsResponse, SegmentResponse

app = FastAPI(
    title="No-OpenCV Image Segmentation API",
    description="FastAPI backend for thresholding and segmentation from scratch using NumPy and Pillow only.",
    version="1.0.0",
)

# During development, allow your React dev server. Tighten this list before deployment.
app.add_middleware(
    CORSMiddleware,
    allow_origins=[
        "http://localhost:3000",
        "http://127.0.0.1:3000",
        "http://localhost:5173",
        "http://127.0.0.1:5173",
    ],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.get("/", response_class=HTMLResponse)
def root() -> str:
    return """
    <h2>No-OpenCV Image Segmentation API</h2>
    <p>Open <a href='/docs'>/docs</a> to test the endpoints.</p>
    <p>Main endpoint: <code>POST /api/segment</code></p>
    """


@app.get("/api/health")
def health() -> dict:
    return {"status": "ok", "opencv_used": False}


@app.get("/api/methods", response_model=MethodsResponse)
def methods() -> MethodsResponse:
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
                description="Local adaptive mean thresholding using an integral image.",
                params={"window_size": 31, "offset": 5},
            ),
        ],
        segmentation=[
            MethodInfo(
                name="kmeans",
                category="segmentation",
                accepts="grayscale or color",
                description="K-means pixel clustering on color features, optionally with x/y coordinates.",
                params={"k": 4, "max_iter": 40, "seed": 0, "include_xy": False},
            ),
            MethodInfo(
                name="region_growing",
                category="segmentation",
                accepts="grayscale or color; converted to grayscale internally",
                description="Queue-based region growing from a seed point or automatic Otsu-based seed.",
                params={"seed_x": None, "seed_y": None, "threshold": 12, "connectivity": 8},
            ),
            MethodInfo(
                name="agglomerative",
                category="segmentation",
                accepts="grayscale or color",
                description="Hierarchical agglomerative clustering on a downsampled feature image, then upscaled.",
                params={"clusters": 4, "sample_size": 32},
            ),
            MethodInfo(
                name="mean_shift",
                category="segmentation",
                accepts="grayscale or color",
                description="Mean-shift clustering using color plus position features on a downsampled image.",
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
        if category == "thresholding":
            if method not in ALLOWED_THRESHOLD_METHODS:
                raise HTTPException(status_code=400, detail=f"Unknown thresholding method: {method}")
            output, metadata = run_thresholding(method, image, params)
        elif category == "segmentation":
            if method not in ALLOWED_SEGMENTATION_METHODS:
                raise HTTPException(status_code=400, detail=f"Unknown segmentation method: {method}")
            output, metadata = run_segmentation(method, image, params)
        else:
            raise HTTPException(status_code=400, detail="category must be thresholding or segmentation")
    except HTTPException:
        raise
    except Exception as exc:
        raise HTTPException(status_code=500, detail=str(exc)) from exc

    return SegmentResponse(
        method=method,
        filename=file.filename or "uploaded-image",
        image_base64=array_to_png_base64(output),
        metadata=metadata,
    )
