import argparse
from utils.image_io import load_image, to_gray, save_image
from algorithms.thresholding import optimal_threshold, otsu_threshold, spectral_threshold, local_threshold
from algorithms.segmentation import kmeans_segmentation, region_growing, agglomerative_segmentation, mean_shift_segmentation


def main():
    p = argparse.ArgumentParser(description="No-OpenCV image thresholding and segmentation")
    p.add_argument("--image", required=True)
    p.add_argument("--method", required=True, choices=[
        "optimal", "otsu", "spectral", "local", "kmeans", "region_growing", "agglomerative", "mean_shift"
    ])
    p.add_argument("--output", required=True)
    p.add_argument("--clusters", type=int, default=4)
    p.add_argument("--levels", type=int, default=3)
    p.add_argument("--window", type=int, default=31)
    p.add_argument("--offset", type=float, default=5)
    p.add_argument("--threshold", type=float, default=12)
    p.add_argument("--seed-x", type=int, default=None)
    p.add_argument("--seed-y", type=int, default=None)
    p.add_argument("--bandwidth", type=float, default=0.18)
    args = p.parse_args()

    image = load_image(args.image, mode="RGB")
    gray = to_gray(image)

    if args.method == "optimal":
        value, out = optimal_threshold(gray)
        print(f"Optimal threshold = {value:.2f}")
    elif args.method == "otsu":
        value, out = otsu_threshold(gray)
        print(f"Otsu threshold = {value}")
    elif args.method == "spectral":
        values, out = spectral_threshold(gray, levels=args.levels)
        print(f"Spectral thresholds = {values}")
    elif args.method == "local":
        out = local_threshold(gray, window_size=args.window, offset=args.offset)
    elif args.method == "kmeans":
        out, _ = kmeans_segmentation(image, k=args.clusters, include_xy=True)
    elif args.method == "region_growing":
        seed = None if args.seed_x is None or args.seed_y is None else (args.seed_x, args.seed_y)
        out = region_growing(gray, seed_point=seed, threshold=args.threshold)
    elif args.method == "agglomerative":
        out, _ = agglomerative_segmentation(image, clusters=args.clusters)
    elif args.method == "mean_shift":
        out, _ = mean_shift_segmentation(image, bandwidth=args.bandwidth)
    else:
        raise ValueError(args.method)

    save_image(out, args.output)
    print(f"Saved: {args.output}")


if __name__ == "__main__":
    main()
