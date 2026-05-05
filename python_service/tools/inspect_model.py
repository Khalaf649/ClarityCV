"""
Inspect a trained_model.pkl so you can see its layout.

Run:
    python inspect_model.py path/to/trained_model.pkl

If the recognizer fails to load your pickle, run this first — it prints
every key, type, shape and a small sample, and tries to suggest which key
corresponds to which expected entry (mean, eigenvectors, projections,
labels, image_shape).
"""
from __future__ import annotations

import pickle
import sys
from pathlib import Path
from typing import Any

import numpy as np


def _describe(value: Any, indent: int = 2) -> str:
    pad = " " * indent
    if isinstance(value, np.ndarray):
        snippet = np.array2string(
            value.flat[:5] if value.size else value, precision=4
        )
        return (
            f"ndarray  shape={value.shape}  dtype={value.dtype}  "
            f"min={value.min() if value.size else 'n/a':.4g}  "
            f"max={value.max() if value.size else 'n/a':.4g}\n"
            f"{pad}first 5 = {snippet}"
        )
    if isinstance(value, (list, tuple)):
        n = len(value)
        head = ", ".join(repr(x) for x in value[:5])
        return f"{type(value).__name__}  len={n}  head=[{head}{', ...' if n > 5 else ''}]"
    if isinstance(value, dict):
        return f"dict  keys={list(value.keys())}"
    if isinstance(value, (int, float, str, bool)):
        return f"{type(value).__name__} = {value!r}"
    if value is None:
        return "None"
    return f"{type(value).__module__}.{type(value).__name__}"


def _guess_role(key: str, value: Any) -> str:
    """Heuristic guess about what a particular field is for."""
    k = key.lower()
    is_array = isinstance(value, np.ndarray)
    is_seq = isinstance(value, (list, tuple))
    is_scalar = isinstance(value, (int, float, bool, str)) or value is None

    # Exact-match heuristics for scalar fields first — `n_components` is just
    # an int, not an eigenvector matrix, even though its name contains
    # "components".
    if is_scalar:
        if k in ("n_components", "k", "num_components"):
            return "scalar — number of principal components kept"
        if k in ("threshold", "thresh", "tau"):
            return "decision threshold"
        if "shape" in k:
            return "image_shape (H, W)"
        return ""  # don't pattern-match a scalar against array roles

    hints = {
        "mean": "mean (subtracted from input before projection)",
        "avg": "mean",
        "mu": "mean",
        "eigen": "eigenvectors / principal components",
        "components": "eigenvectors",
        "basis": "eigenvectors",
        "projection": "training projections (one row per training image)",
        "feature": "training projections",
        "weight": "training projections",
        "embedding": "training projections",
        "label": "training labels",
        "target": "training labels",
        "name": "labels or label name map",
        "shape": "image_shape (H, W)",
    }
    for needle, role in hints.items():
        if needle in k:
            return role
    if is_array:
        if value.ndim == 1 and value.size > 100:
            return "1-D vector — possibly mean (length = H*W)"
        if value.ndim == 2:
            return (
                f"2-D matrix {value.shape} — could be eigenvectors, projections, "
                "or covariance"
            )
    if is_seq and len(value) > 0 and hasattr(value[0], "shape"):
        return "list of arrays — possibly raw training images"
    return ""


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: python inspect_model.py path/to/trained_model.pkl")
        return 2

    path = Path(sys.argv[1])
    if not path.exists():
        print(f"ERROR: file not found: {path}")
        return 1

    print(f"Reading {path.resolve()} ({path.stat().st_size:,} bytes)")
    print()

    with open(path, "rb") as f:
        obj = pickle.load(f)

    print(f"Top-level type: {type(obj).__module__}.{type(obj).__name__}")
    print()

    if isinstance(obj, dict):
        print(f"Dict has {len(obj)} keys:")
        for k in obj:
            print(f"  {k!r}")
            print(f"    {_describe(obj[k], indent=6)}")
            role = _guess_role(str(k), obj[k])
            if role:
                print(f"    ↳ likely: {role}")
            print()
    elif isinstance(obj, (list, tuple)):
        print(f"{type(obj).__name__} with {len(obj)} items:")
        for i, item in enumerate(obj):
            print(f"  [{i}]")
            print(f"    {_describe(item, indent=6)}")
            print()
    else:
        print("Object attributes:")
        for attr in dir(obj):
            if attr.startswith("_"):
                continue
            try:
                val = getattr(obj, attr)
            except Exception:
                continue
            if callable(val):
                continue
            print(f"  {attr}: {_describe(val, indent=6)}")
            print()

    print("If recognizer.py can't load this file, copy the printed key names")
    print("into MODEL_KEY_ALIASES at the top of recognizer.py and re-run.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
