"""
Eigenfaces / PCA face recognizer.

This module loads a pre-trained PCA model from a pickle file and uses it to
recognize faces. The loader is intentionally tolerant — it tries several
common key-naming conventions because different training scripts (sklearn-
based, hand-rolled, etc.) save the model in slightly different shapes.

If your .pkl uses a layout the loader doesn't understand, run
`python inspect_model.py path/to/trained_model.pkl` and add the missing key
names in MODEL_KEY_ALIASES below.
"""
from __future__ import annotations

import pickle
from dataclasses import dataclass
from pathlib import Path
from typing import Any, List, Optional, Sequence, Tuple

import numpy as np

# ---------------------------------------------------------------------------
# Key aliases used while introspecting the unpickled object
# ---------------------------------------------------------------------------

MODEL_KEY_ALIASES = {
    "mean": ["mean", "mean_face", "avg_face", "mu", "average", "mean_vector",
             "mean_image"],
    "eigenvectors": [
        "eigenfaces", "eigenvectors", "eigen_vectors", "eigvecs", "eigenvecs",
        "components", "components_", "V", "Vt", "pca_components", "U",
        "basis", "principal_components",
    ],
    "projections": [
        "projections", "train_projections", "train_features", "features",
        "coefficients", "weights", "signatures", "X_train_pca", "embeddings",
        "train_embeddings", "dataset_projections", "train_proj",
    ],
    "labels": [
        "labels", "y", "y_train", "targets", "names", "class_names",
        "train_labels", "identities", "ids",
    ],
    "image_shape": ["image_shape", "face_shape", "shape", "img_shape",
                    "input_shape", "size"],
    "threshold": ["threshold", "thresh", "tau", "default_threshold"],
    "label_names": ["label_names", "class_names", "id_to_name", "name_map",
                    "names_map"],
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _first_present(d: dict, names: Sequence[str]) -> Optional[Any]:
    for name in names:
        if name in d:
            return d[name]
    return None


def _to_numpy(x: Any) -> np.ndarray:
    arr = np.asarray(x)
    if arr.dtype == object:
        # try to stack a list of arrays
        arr = np.stack([np.asarray(item) for item in x])
    return arr


def _normalize_eigenvectors(eig: np.ndarray, n_pixels: int) -> np.ndarray:
    """Return eigenvectors as shape (k, n_pixels). Many training scripts save
    them as (n_pixels, k) instead — detect and transpose."""
    if eig.ndim != 2:
        raise ValueError(f"Eigenvectors must be 2-D, got shape {eig.shape}")
    if eig.shape[1] == n_pixels:
        return eig
    if eig.shape[0] == n_pixels:
        return eig.T
    raise ValueError(
        f"Eigenvector shape {eig.shape} doesn't match flattened image size {n_pixels}."
    )


def _infer_image_shape(
    data: Any,
    mean: Optional[np.ndarray] = None,
) -> Optional[Tuple[int, int]]:
    """When the pickle doesn't store image_shape, try to recover it from
    sibling fields: a 2-D image in train_images, or assume the mean is square."""
    if isinstance(data, dict):
        for key in ("train_images", "images", "X_train"):
            seq = data.get(key)
            if seq is None:
                continue
            try:
                first = seq[0] if hasattr(seq, "__len__") else None
            except Exception:
                first = None
            if first is None:
                continue
            arr = np.asarray(first)
            if arr.ndim == 2:
                return (int(arr.shape[0]), int(arr.shape[1]))
            if arr.ndim == 3 and arr.shape[-1] in (1, 3, 4):
                return (int(arr.shape[0]), int(arr.shape[1]))

    # Fallback: assume the flattened image is square (very common).
    if mean is not None:
        n = int(np.asarray(mean).size)
        side = int(round(np.sqrt(n)))
        if side * side == n:
            return (side, side)
    return None


# ---------------------------------------------------------------------------
# Result containers
# ---------------------------------------------------------------------------

@dataclass
class Recognition:
    """A single recognition result."""
    label: str
    distance: float
    confidence: float  # 0..1, higher is better
    is_match: bool
    nearest_index: int


# ---------------------------------------------------------------------------
# Recognizer
# ---------------------------------------------------------------------------

class EigenfaceRecognizer:
    """PCA / Eigenfaces classifier. Recognition is nearest-neighbour in the
    eigenface subspace (Euclidean by default)."""

    def __init__(
        self,
        mean: np.ndarray,
        eigenvectors: np.ndarray,
        projections: np.ndarray,
        labels: Sequence[Any],
        image_shape: Tuple[int, int],
        default_threshold: Optional[float] = None,
        label_names: Optional[dict] = None,
    ) -> None:
        n_pixels = int(np.prod(image_shape))

        self.mean = mean.astype(np.float64).reshape(-1)
        if self.mean.size != n_pixels:
            raise ValueError(
                f"Mean vector has {self.mean.size} entries but image_shape "
                f"{image_shape} expects {n_pixels}."
            )

        self.eigenvectors = _normalize_eigenvectors(
            eigenvectors.astype(np.float64), n_pixels
        )
        self.image_shape = (int(image_shape[0]), int(image_shape[1]))

        self.projections = projections.astype(np.float64)
        if self.projections.ndim != 2:
            raise ValueError(
                f"Projections must be 2-D, got shape {self.projections.shape}"
            )
        # Make sure projections are (n_samples, k) — transpose if needed.
        k = self.eigenvectors.shape[0]
        if self.projections.shape[1] != k and self.projections.shape[0] == k:
            self.projections = self.projections.T

        self.labels = list(labels)
        if len(self.labels) != self.projections.shape[0]:
            raise ValueError(
                f"Got {len(self.labels)} labels but {self.projections.shape[0]} "
                "training projections."
            )

        self.label_names = label_names or {}
        self.default_threshold = default_threshold

    # -- model loading -----------------------------------------------------

    @classmethod
    def from_pickle(cls, path: str | Path) -> "EigenfaceRecognizer":
        path = Path(path)
        if not path.exists():
            raise FileNotFoundError(f"Model file not found: {path}")
        with open(path, "rb") as f:
            data = pickle.load(f)
        return cls.from_object(data)

    @classmethod
    def from_object(cls, data: Any) -> "EigenfaceRecognizer":
        """Build a recognizer from whatever was stored in the pickle.

        Supports:
          • dict with keys following any of the aliases in MODEL_KEY_ALIASES
          • dict containing an sklearn PCA under 'pca' / 'model' plus labels
          • tuple/list (mean, eigenvectors, projections, labels[, shape])
        """
        if isinstance(data, dict):
            # If the dict carries an sklearn PCA, pull components from it.
            if "pca" in data or "model" in data:
                pca_obj = data.get("pca", data.get("model"))
                mean = getattr(pca_obj, "mean_", None)
                eig = getattr(pca_obj, "components_", None)
                if mean is None or eig is None:
                    raise ValueError(
                        "Dict has 'pca'/'model' but it doesn't expose mean_/components_."
                    )
                projections = _first_present(data, MODEL_KEY_ALIASES["projections"])
                if projections is None:
                    raise ValueError(
                        "Couldn't find training projections in the model dict. "
                        "Expected one of: " + ", ".join(MODEL_KEY_ALIASES["projections"])
                    )
                labels = _first_present(data, MODEL_KEY_ALIASES["labels"])
                if labels is None:
                    raise ValueError("Couldn't find labels in the model dict.")
                shape = _first_present(data, MODEL_KEY_ALIASES["image_shape"])
                if shape is None:
                    raise ValueError(
                        "Couldn't find image_shape — store it in the pickle next time, "
                        "or add the key name to MODEL_KEY_ALIASES['image_shape']."
                    )
                threshold = _first_present(data, MODEL_KEY_ALIASES["threshold"])
                label_names = _first_present(data, MODEL_KEY_ALIASES["label_names"])
                return cls(
                    mean=_to_numpy(mean),
                    eigenvectors=_to_numpy(eig),
                    projections=_to_numpy(projections),
                    labels=list(labels),
                    image_shape=tuple(shape),
                    default_threshold=float(threshold) if threshold is not None else None,
                    label_names=label_names if isinstance(label_names, dict) else None,
                )

            mean = _first_present(data, MODEL_KEY_ALIASES["mean"])
            eig = _first_present(data, MODEL_KEY_ALIASES["eigenvectors"])
            projections = _first_present(data, MODEL_KEY_ALIASES["projections"])
            labels = _first_present(data, MODEL_KEY_ALIASES["labels"])
            shape = _first_present(data, MODEL_KEY_ALIASES["image_shape"])
            threshold = _first_present(data, MODEL_KEY_ALIASES["threshold"])
            label_names = _first_present(data, MODEL_KEY_ALIASES["label_names"])

            # image_shape isn't always saved — try to recover it.
            if shape is None and mean is not None:
                shape = _infer_image_shape(data, mean=_to_numpy(mean))

            missing = []
            if mean is None: missing.append("mean")
            if eig is None: missing.append("eigenvectors")
            if projections is None: missing.append("projections")
            if labels is None: missing.append("labels")
            if shape is None: missing.append("image_shape (and couldn't infer)")
            if missing:
                raise ValueError(
                    "Pickle dict is missing required entries: "
                    + ", ".join(missing)
                    + ". Run inspect_model.py to see what keys are present, then "
                    "add aliases to recognizer.MODEL_KEY_ALIASES."
                )

            return cls(
                mean=_to_numpy(mean),
                eigenvectors=_to_numpy(eig),
                projections=_to_numpy(projections),
                labels=list(labels),
                image_shape=tuple(shape),
                default_threshold=float(threshold) if threshold is not None else None,
                label_names=label_names if isinstance(label_names, dict) else None,
            )

        if isinstance(data, (tuple, list)):
            if len(data) < 5:
                raise ValueError(
                    f"Tuple/list pickle must hold at least "
                    f"(mean, eigenvectors, projections, labels, image_shape); "
                    f"got {len(data)} items."
                )
            mean, eig, projections, labels, shape = data[:5]
            threshold = data[5] if len(data) > 5 else None
            return cls(
                mean=_to_numpy(mean),
                eigenvectors=_to_numpy(eig),
                projections=_to_numpy(projections),
                labels=list(labels),
                image_shape=tuple(shape),
                default_threshold=float(threshold) if threshold is not None else None,
            )

        raise TypeError(
            f"Unsupported pickle payload type: {type(data).__name__}. "
            "Expected dict, tuple, or list."
        )

    # -- core ops ----------------------------------------------------------

    def project(self, face_gray: np.ndarray) -> np.ndarray:
        """Project a (H, W) grayscale face into eigenface coordinates."""
        if face_gray.shape != self.image_shape:
            raise ValueError(
                f"Expected face shape {self.image_shape}, got {face_gray.shape}. "
                "Resize before calling project()."
            )
        flat = face_gray.astype(np.float64).reshape(-1) - self.mean
        return self.eigenvectors @ flat

    def recognize(
        self,
        face_gray: np.ndarray,
        threshold: Optional[float] = None,
    ) -> Recognition:
        """Find the closest training face and decide if it's a match.

        Uses the same confidence formula as the original training script
        (face_recognition.py / FaceRecognition.recognize_face):

            confidence = 1 - (min_dist / max_dist_for_this_probe)
            is_match   = confidence >= threshold

        `threshold` is on confidence (0..1). If None, falls back to the
        threshold stored in the pickle, or 0.5 otherwise.
        """
        proj = self.project(face_gray)
        diffs = self.projections - proj
        distances = np.linalg.norm(diffs, axis=1)
        idx = int(np.argmin(distances))
        min_dist = float(distances[idx])
        max_dist = float(np.max(distances))

        # Per-probe normalised confidence — matches the original training code
        # exactly so the threshold the user trained with (e.g. 0.85) is
        # directly applicable here.
        confidence = 1.0 - (min_dist / max_dist) if max_dist > 0 else 1.0

        cutoff = self._resolve_threshold(threshold)
        label = str(self.labels[idx])
        if isinstance(self.label_names, dict) and label in self.label_names:
            label = str(self.label_names[label])

        is_match = confidence + 1e-9 >= cutoff
        if not is_match:
            label = "Unknown"

        return Recognition(
            label=label,
            distance=min_dist,
            confidence=confidence,
            is_match=is_match,
            nearest_index=idx,
        )

    # -- helpers -----------------------------------------------------------

    def _resolve_threshold(self, value: Optional[float]) -> float:
        """The UI slider is 1..10. Map it linearly onto a confidence cutoff
        in [0.1, 1.0] so the user can dial in the exact threshold they
        trained with (e.g. slider = 8.5 ↔ confidence cutoff 0.85).

        If `value` is None, fall back to the threshold stored inside the
        pickle (FaceRecognition saves 0.85 by default), or 0.5 if absent.
        """
        if value is None:
            if self.default_threshold is not None:
                return float(self.default_threshold)
            return 0.5
        slider = max(0.0, min(float(value), 10.0))
        # Special case: a value already in [0,1] is already a confidence.
        # (Lets evaluate.py and library callers pass raw confidence values.)
        if 0.0 <= slider <= 1.0 and slider != 1.0:
            return slider
        return slider / 10.0

    @property
    def n_components(self) -> int:
        return int(self.eigenvectors.shape[0])

    @property
    def n_train(self) -> int:
        return int(self.projections.shape[0])

    @property
    def unique_labels(self) -> List[str]:
        seen = []
        for lab in self.labels:
            s = str(lab)
            if s not in seen:
                seen.append(s)
        return seen
