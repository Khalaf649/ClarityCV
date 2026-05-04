from pathlib import Path
from PIL import Image
import numpy as np


def load_image(path, mode="RGB"):
    """Load an image as a NumPy array without OpenCV."""
    img = Image.open(path)
    if mode:
        img = img.convert(mode)
    return np.asarray(img)


def to_gray(image):
    """Convert RGB/RGBA/grayscale NumPy image to uint8 grayscale."""
    arr = np.asarray(image)
    if arr.ndim == 2:
        gray = arr.astype(np.float32)
    else:
        rgb = arr[..., :3].astype(np.float32)
        gray = 0.299 * rgb[..., 0] + 0.587 * rgb[..., 1] + 0.114 * rgb[..., 2]
    return np.clip(gray, 0, 255).astype(np.uint8)


def normalize_uint8(arr):
    arr = np.asarray(arr, dtype=np.float32)
    mn, mx = float(arr.min()), float(arr.max())
    if mx <= mn:
        return np.zeros(arr.shape, dtype=np.uint8)
    return ((arr - mn) * 255.0 / (mx - mn)).astype(np.uint8)


def save_image(array, path):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    arr = np.asarray(array)
    if arr.dtype != np.uint8:
        arr = normalize_uint8(arr)
    Image.fromarray(arr).save(path)


def resize_keep_aspect(arr, max_size):
    img = Image.fromarray(arr.astype(np.uint8))
    img.thumbnail(max_size)
    return np.asarray(img)
