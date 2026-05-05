"""Eigenfaces / PCA face-recognition core.

Pure algorithms — no HTTP, no filesystem, no CLI. The :mod:`api` and
:mod:`tools` packages depend on this; nothing in here depends on them.
"""

from .detector import FaceBox, FaceDetector, preprocess_face
from .pipeline import FaceRecognitionPipeline, FaceResult, draw_results
from .recognizer import EigenfaceRecognizer, Recognition

__all__ = [
    "FaceBox",
    "FaceDetector",
    "FaceRecognitionPipeline",
    "FaceResult",
    "EigenfaceRecognizer",
    "Recognition",
    "draw_results",
    "preprocess_face",
]
