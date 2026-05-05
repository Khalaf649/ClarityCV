"""
End-to-end face-recognition pipeline.

  detect → crop → resize → recognize → draw boxes & labels on the output image.
"""
from __future__ import annotations

import time
from dataclasses import dataclass
from typing import List, Optional, Tuple

import cv2
import numpy as np

from .detector import FaceBox, FaceDetector, preprocess_face
from .recognizer import EigenfaceRecognizer, Recognition


@dataclass
class FaceResult:
    box: FaceBox
    recognition: Recognition


def _color_for(rec: Recognition) -> Tuple[int, int, int]:
    # BGR — OpenCV convention. Green for matches, amber for unsure-but-close,
    # red for unknown.
    if not rec.is_match:
        return (60, 60, 220)  # red-ish
    if rec.confidence < 0.4:
        return (40, 180, 230)  # amber
    return (80, 200, 80)  # green


def draw_results(
    image: np.ndarray,
    results: List[FaceResult],
    detect_only: bool = False,
) -> np.ndarray:
    """Annotate the input image with bounding boxes and labels.
    Expects BGR uint8; returns BGR uint8.

    When `detect_only` is True, draws boxes without identity labels —
    used when the UI is in 'Face Detection' mode rather than recognition.
    """
    out = image.copy()
    if out.ndim == 2:
        out = cv2.cvtColor(out, cv2.COLOR_GRAY2BGR)

    for r in results:
        x1, y1, x2, y2 = r.box.as_xyxy()
        if detect_only:
            color = (80, 200, 80)  # green box for every detection
        else:
            color = _color_for(r.recognition)
        cv2.rectangle(out, (x1, y1), (x2, y2), color, 2)

        if detect_only:
            continue

        label = r.recognition.label
        conf_pct = int(round(r.recognition.confidence * 100))
        text = f"{label}  {conf_pct}%"

        # Label background for readability.
        font = cv2.FONT_HERSHEY_SIMPLEX
        scale = max(0.45, min(0.9, (x2 - x1) / 220.0))
        thickness = 1 if scale < 0.7 else 2
        (tw, th), baseline = cv2.getTextSize(text, font, scale, thickness)

        label_h = th + baseline + 6
        # Default: place the label band ABOVE the face box. If there's no
        # room (face touches the top of the image), drop it INSIDE at the top
        # of the box so it's still visible.
        if y1 >= label_h:
            bg_y1, bg_y2 = y1 - label_h, y1
        else:
            bg_y1, bg_y2 = y1, min(y1 + label_h, out.shape[0])
        bg_x2 = min(x1 + tw + 8, out.shape[1])

        cv2.rectangle(out, (x1, bg_y1), (bg_x2, bg_y2), color, -1)
        cv2.putText(
            out,
            text,
            (x1 + 4, bg_y2 - baseline - 2),
            font,
            scale,
            (255, 255, 255),
            thickness,
            cv2.LINE_AA,
        )
    return out


class FaceRecognitionPipeline:
    def __init__(
        self,
        recognizer: EigenfaceRecognizer,
        detector: Optional[FaceDetector] = None,
    ) -> None:
        self.recognizer = recognizer
        self.detector = detector or FaceDetector()

    def run(
        self,
        image_bgr: np.ndarray,
        threshold: Optional[float] = None,
        detect_only: bool = False,
    ) -> Tuple[np.ndarray, List[FaceResult], float]:
        """Process an image. Returns (annotated_image, results, elapsed_seconds).

        When `detect_only` is True, runs only face detection — skips the PCA
        projection step and returns boxes without identity labels.
        """
        t0 = time.perf_counter()

        boxes = self.detector.detect(image_bgr)
        results: List[FaceResult] = []

        if detect_only:
            # Cheap path: skip projection. Still return one FaceResult per box
            # with an empty Recognition so callers can count consistently.
            for box in boxes:
                results.append(
                    FaceResult(
                        box=box,
                        recognition=Recognition(
                            label="face",
                            distance=0.0,
                            confidence=1.0,
                            is_match=True,
                            nearest_index=-1,
                        ),
                    )
                )
        else:
            gray_full = self.detector.to_gray(image_bgr)
            for box in boxes:
                face = box.crop(gray_full)
                if face.size == 0:
                    continue
                face_resized = preprocess_face(face, self.recognizer.image_shape)
                rec = self.recognizer.recognize(face_resized, threshold=threshold)
                results.append(FaceResult(box=box, recognition=rec))

        annotated = draw_results(image_bgr, results, detect_only=detect_only)
        elapsed = time.perf_counter() - t0
        return annotated, results, elapsed
