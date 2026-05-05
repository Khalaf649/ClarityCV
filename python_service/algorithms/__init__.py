"""Segmentation and thresholding algorithms.

Pure NumPy + Pillow — no OpenCV dependency. Each function takes a NumPy
array and returns one (label maps for segmentation, binary masks for
thresholding) plus optional metadata.
"""

from .segmentation import (
    agglomerative_segmentation,
    kmeans_segmentation,
    mean_shift_segmentation,
    region_growing,
)
from .thresholding import (
    binary_from_threshold,
    local_threshold,
    optimal_threshold,
    otsu_threshold,
    spectral_threshold,
)

__all__ = [
    "agglomerative_segmentation",
    "binary_from_threshold",
    "kmeans_segmentation",
    "local_threshold",
    "mean_shift_segmentation",
    "optimal_threshold",
    "otsu_threshold",
    "region_growing",
    "spectral_threshold",
]
