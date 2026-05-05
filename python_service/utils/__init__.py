"""Image-IO utilities used by segmentation algorithms.

Pure Pillow + NumPy — no OpenCV. Kept separate from the recognition
service's image handling because the recognition path needs OpenCV's
Haar cascade while these algorithms intentionally don't touch OpenCV.
"""

from .image_io import load_image, save_image, to_gray
from .palette import labels_to_random_colors

__all__ = ["load_image", "save_image", "to_gray", "labels_to_random_colors"]
