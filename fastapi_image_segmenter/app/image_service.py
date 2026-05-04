from __future__ import annotations

import base64
from io import BytesIO
from typing import Any, Dict, Tuple

import numpy as np
from PIL import Image

from algorithms.thresholding import (
    local_threshold,
    optimal_threshold,
    otsu_threshold,
    spectral_threshold,
)
from algorithms.segmentation import (
    agglomerative_segmentation,
    kmeans_segmentation,
    mean_shift_segmentation,
    region_growing,
)
from utils.image_io import to_gray


ALLOWED_THRESHOLD_METHODS = {"optimal", "otsu", "spectral", "local"}
ALLOWED_SEGMENTATION_METHODS = {"kmeans", "region_growing", "agglomerative", "mean_shift"}


def read_upload_to_array(file_bytes: bytes, mode: str = "RGB") -> np.ndarray:
    """Read an uploaded image into a NumPy array using Pillow, not OpenCV."""
    buffer = BytesIO(file_bytes)
    buffer.seek(0)  # Ensure we're at the beginning
    image = Image.open(buffer)
    image.load()  # Force load the image data while buffer is still open
    if mode:
        image = image.convert(mode)
    return np.asarray(image)


def array_to_png_base64(array: np.ndarray) -> str:
    """Encode a NumPy image array as PNG base64 for JSON API responses."""
    arr = np.asarray(array)
    if arr.dtype != np.uint8:
        arr = np.clip(arr, 0, 255).astype(np.uint8)
    if arr.ndim == 3 and arr.shape[-1] == 1:
        arr = arr[..., 0]
    buffer = BytesIO()
    Image.fromarray(arr).save(buffer, format="PNG")
    return base64.b64encode(buffer.getvalue()).decode("utf-8")


def run_thresholding(method: str, image: np.ndarray, params: Dict[str, Any]) -> Tuple[np.ndarray, Dict[str, Any]]:
    gray = to_gray(image)

    if method == "optimal":
        threshold, output = optimal_threshold(
            gray,
            epsilon=float(params.get("epsilon", 0.5)),
            max_iter=int(params.get("max_iter", 100)),
        )
        return output, {"threshold": float(threshold)}

    if method == "otsu":
        threshold, output = otsu_threshold(gray)
        return output, {"threshold": int(threshold)}

    if method == "spectral":
        thresholds, output = spectral_threshold(gray, levels=int(params.get("levels", 3)))
        return output, {"thresholds": [int(t) for t in thresholds], "levels": int(params.get("levels", 3))}

    if method == "local":
        output = local_threshold(
            gray,
            window_size=int(params.get("window_size", 31)),
            offset=float(params.get("offset", 5)),
        )
        return output, {"window_size": int(params.get("window_size", 31)), "offset": float(params.get("offset", 5))}

    raise ValueError(f"Unknown thresholding method: {method}")


def run_segmentation(method: str, image: np.ndarray, params: Dict[str, Any]) -> Tuple[np.ndarray, Dict[str, Any]]:
    if method == "kmeans":
        output, labels = kmeans_segmentation(
            image,
            k=int(params.get("k", 4)),
            max_iter=int(params.get("max_iter", 40)),
            seed=int(params.get("seed", 0)),
            include_xy=bool(params.get("include_xy", False)),
        )
        return output, {"clusters": int(params.get("k", 4)), "labels_count": int(len(np.unique(labels)))}

    if method == "region_growing":
        seed_x = params.get("seed_x")
        seed_y = params.get("seed_y")
        seed_point = None
        if seed_x is not None and seed_y is not None:
            seed_point = (int(seed_x), int(seed_y))
        output = region_growing(
            image,
            seed_point=seed_point,
            threshold=float(params.get("threshold", 12)),
            connectivity=int(params.get("connectivity", 8)),
        )
        return output, {"seed_point": seed_point, "threshold": float(params.get("threshold", 12))}

    if method == "agglomerative":
        output, labels = agglomerative_segmentation(
            image,
            clusters=int(params.get("clusters", 4)),
            sample_size=int(params.get("sample_size", 32)),
        )
        return output, {"clusters": int(params.get("clusters", 4)), "labels_count": int(len(np.unique(labels)))}

    if method == "mean_shift":
        output, labels = mean_shift_segmentation(
            image,
            bandwidth=float(params.get("bandwidth", 0.18)),
            max_iter=int(params.get("max_iter", 6)),
            sample_size=int(params.get("sample_size", 48)),
            merge_radius=params.get("merge_radius"),
        )
        return output, {"labels_count": int(len(np.unique(labels))), "bandwidth": float(params.get("bandwidth", 0.18))}

    raise ValueError(f"Unknown segmentation method: {method}")
