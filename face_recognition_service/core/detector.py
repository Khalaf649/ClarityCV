"""
Face detection with OpenCV Haar cascades.

We use OpenCV here because writing a Viola–Jones detector from scratch is a
large project on its own and the assignment is about *recognition*. The
cascade XML ships with opencv-python so there's nothing extra to download.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import List, Optional, Tuple

import cv2
import numpy as np


@dataclass
class FaceBox:
    x: int
    y: int
    w: int
    h: int

    def crop(self, image: np.ndarray) -> np.ndarray:
        return image[self.y : self.y + self.h, self.x : self.x + self.w]

    def as_xyxy(self) -> Tuple[int, int, int, int]:
        return self.x, self.y, self.x + self.w, self.y + self.h


class FaceDetector:
    """Wrapper around cv2.CascadeClassifier with a small fallback for when
    the cascade misses an obvious face (e.g. tightly cropped portraits)."""

    def __init__(
        self,
        cascade_path: Optional[str] = None,
        scale_factor: float = 1.1,
        min_neighbors: int = 5,
        min_size: Tuple[int, int] = (30, 30),
    ) -> None:
        if cascade_path is None:
            cascade_path = (
                cv2.data.haarcascades + "haarcascade_frontalface_default.xml"
            )
        self.cascade = cv2.CascadeClassifier(cascade_path)
        if self.cascade.empty():
            raise RuntimeError(f"Failed to load Haar cascade at {cascade_path}")
        self.scale_factor = scale_factor
        self.min_neighbors = min_neighbors
        self.min_size = min_size

    def detect(self, image: np.ndarray) -> List[FaceBox]:
        gray = self.to_gray(image)
        gray_eq = cv2.equalizeHist(gray)
        rects = self.cascade.detectMultiScale(
            gray_eq,
            scaleFactor=self.scale_factor,
            minNeighbors=self.min_neighbors,
            minSize=self.min_size,
        )
        boxes = [FaceBox(int(x), int(y), int(w), int(h)) for (x, y, w, h) in rects]

        # Fallback: if nothing detected and the image is small/portrait-like,
        # treat the whole image as one face. This keeps recognition usable on
        # already-cropped face datasets (ORL / Yale / AT&T style).
        if not boxes:
            h, w = gray.shape[:2]
            aspect = w / h if h else 1.0
            if 0.6 < aspect < 1.4 and max(h, w) <= 512:
                boxes = [FaceBox(0, 0, w, h)]

        return boxes

    @staticmethod
    def to_gray(image: np.ndarray) -> np.ndarray:
        if image.ndim == 2:
            return image
        if image.ndim == 3 and image.shape[2] == 3:
            return cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        if image.ndim == 3 and image.shape[2] == 4:
            return cv2.cvtColor(image, cv2.COLOR_BGRA2GRAY)
        raise ValueError(f"Unsupported image shape for face detection: {image.shape}")

    # Backwards-compatible alias for older callers.
    _to_gray = to_gray


def preprocess_face(
    face_gray: np.ndarray,
    target_shape: Tuple[int, int],
    equalize: bool = False,
) -> np.ndarray:
    """Resize to target_shape (H, W) and optionally histogram-equalise.
    Returns uint8 grayscale, the format eigenfaces expect.

    NOTE: Defaults to NO equalization, because the typical PCA training
    script (and the one shipped with this project) does plain resize+grayscale
    only. If you trained with equalization, pass equalize=True so inference
    matches training — otherwise accuracy drops sharply.
    """
    if face_gray.ndim != 2:
        face_gray = cv2.cvtColor(face_gray, cv2.COLOR_BGR2GRAY)
    h, w = target_shape
    resized = cv2.resize(face_gray, (w, h), interpolation=cv2.INTER_AREA)
    if equalize:
        resized = cv2.equalizeHist(resized)
    return resized
