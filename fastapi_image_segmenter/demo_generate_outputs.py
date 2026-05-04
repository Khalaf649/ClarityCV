from pathlib import Path
import numpy as np
from PIL import Image

from utils.image_io import to_gray, save_image
from algorithms.thresholding import optimal_threshold, otsu_threshold, spectral_threshold, local_threshold
from algorithms.segmentation import kmeans_segmentation, region_growing, agglomerative_segmentation, mean_shift_segmentation


def synthetic_image():
    h, w = 180, 240
    yy, xx = np.mgrid[0:h, 0:w]
    img = np.zeros((h, w, 3), dtype=np.uint8)
    img[..., 0] = (xx / w * 255).astype(np.uint8)
    img[..., 1] = (yy / h * 255).astype(np.uint8)
    img[..., 2] = 120
    circle = (xx - 80) ** 2 + (yy - 90) ** 2 < 45 ** 2
    rect = (xx > 140) & (xx < 215) & (yy > 40) & (yy < 140)
    img[circle] = [230, 60, 60]
    img[rect] = [40, 210, 100]
    return img


def main():
    outdir = Path("results")
    outdir.mkdir(exist_ok=True)
    img = synthetic_image()
    save_image(img, outdir / "synthetic_input.png")
    gray = to_gray(img)
    save_image(optimal_threshold(gray)[1], outdir / "optimal.png")
    save_image(otsu_threshold(gray)[1], outdir / "otsu.png")
    save_image(spectral_threshold(gray, levels=4)[1], outdir / "spectral.png")
    save_image(local_threshold(gray, 31, 5), outdir / "local.png")
    save_image(kmeans_segmentation(img, k=4, include_xy=True)[0], outdir / "kmeans.png")
    save_image(region_growing(gray, seed_point=(80, 90), threshold=20), outdir / "region_growing.png")
    save_image(agglomerative_segmentation(img, clusters=4)[0], outdir / "agglomerative.png")
    save_image(mean_shift_segmentation(img, bandwidth=0.18)[0], outdir / "mean_shift.png")
    print("Demo outputs saved in results/")


if __name__ == "__main__":
    main()
