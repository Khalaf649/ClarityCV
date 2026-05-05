"""Pydantic request and response models for the face-recognition API.

Kept in its own module so the schemas can be imported from tests, the
frontend's TypeScript codegen, or third-party clients without dragging in
the FastAPI app and its heavy dependencies.
"""
from __future__ import annotations

from typing import Any, Dict, List

from pydantic import BaseModel, Field


class RecognizeRequest(BaseModel):
    """Body of ``POST /api/recognize_faces``."""

    image: str = Field(
        ...,
        description="Base64-encoded image. Either raw base64 or a data URI "
                    "(``data:image/png;base64,...``) is accepted.",
    )
    method: str = Field(
        "face_recognition",
        description="Either ``face_recognition`` (full PCA identification) "
                    "or ``haar_detection`` (boxes only, no labels).",
    )
    threshold: float = Field(
        5.0,
        description="UI slider value in 1..10. Mapped to a confidence cutoff "
                    "by the recognizer (slider×0.1 ≈ confidence threshold).",
    )


class RecognizeResponse(BaseModel):
    """Body of a successful ``POST /api/recognize_faces`` response.

    The field names use camelCase because the frontend (TypeScript) consumes
    them directly without a transformer.
    """

    success: bool
    image: str = Field(..., description="Base64-encoded annotated PNG.")
    facesDetected: int = Field(
        ...,
        description="In recognition mode this is the number of *matched* "
                    "faces (i.e. confidence ≥ threshold). In detection mode "
                    "it's every face the cascade found.",
    )
    computationTime: float = Field(..., description="Server-side processing time in seconds.")


class HealthResponse(BaseModel):
    """Body of ``GET /api/health``. Returned with status 200 only when the
    model is fully loaded; with 503 (and ``model_loaded=False``) otherwise."""

    status: str
    model_loaded: bool
    n_components: int | None = None
    n_train: int | None = None
    n_classes: int | None = None
    image_shape: list[int] | None = None
    error: str | None = None


# ---------------------------------------------------------------------------
# Segmentation / thresholding endpoints (merged from fastapi_image_segmenter)
# ---------------------------------------------------------------------------

class SegmentResponse(BaseModel):
    """Body of a successful ``POST /api/segment`` response."""

    method: str
    filename: str
    mime_type: str = "image/png"
    image_base64: str
    metadata: Dict[str, Any] = Field(default_factory=dict)


class MethodInfo(BaseModel):
    """Description of a single thresholding/segmentation method."""

    name: str
    category: str
    accepts: str
    description: str
    params: Dict[str, Any]


class MethodsResponse(BaseModel):
    """Body of ``GET /api/methods``. Lists every available segmentation /
    thresholding method and its tunable parameters."""

    thresholding: List[MethodInfo]
    segmentation: List[MethodInfo]
