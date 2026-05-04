from collections import deque
import numpy as np
from PIL import Image
from .thresholding import otsu_threshold


def _as_features(image, include_xy=False, spatial_weight=0.2):
    arr = np.asarray(image).astype(np.float32)

    if arr.ndim == 2:
        base = arr[..., None]
    else:
        base = arr[..., :3]

    h, w = base.shape[:2]
    color = base.reshape(-1, base.shape[-1]) / 255.0

    if not include_xy:
        return color

    yy, xx = np.mgrid[0:h, 0:w]
    xy = np.stack(
        [yy / max(h - 1, 1), xx / max(w - 1, 1)],
        axis=-1
    ).reshape(-1, 2)

    return np.concatenate([color, spatial_weight * xy], axis=1)


def red_green_from_labels(image, labels):
    """
    Convert clustered labels to red/green visualization.

    Darkest cluster = foreground/object = red
    Other clusters = background = green

    This is useful for examples where the object is darker than the background.
    """
    arr = np.asarray(image)

    if arr.ndim == 3:
        gray = np.mean(arr[..., :3], axis=2)
    else:
        gray = arr.astype(np.float32)

    labels = np.asarray(labels)
    unique_labels = np.unique(labels)

    darkest_label = unique_labels[0]
    darkest_mean = float("inf")

    for label in unique_labels:
        mask = labels == label
        if np.any(mask):
            mean_intensity = gray[mask].mean()
            if mean_intensity < darkest_mean:
                darkest_mean = mean_intensity
                darkest_label = label

    out = np.zeros((labels.shape[0], labels.shape[1], 3), dtype=np.uint8)

    # Background = green
    out[:, :] = [0, 255, 0]

    # Foreground/object = red
    out[labels == darkest_label] = [255, 0, 0]

    return out


def red_green_from_mask(mask):
    """
    Convert a binary mask to red/green visualization.

    Mask/object = red
    Background = green
    """
    mask = np.asarray(mask)

    out = np.zeros((mask.shape[0], mask.shape[1], 3), dtype=np.uint8)

    # Background = green
    out[:, :] = [0, 255, 0]

    # Object = red
    out[mask > 0] = [255, 0, 0]

    return out


def _kmeans_plusplus_init(X, k, rng):
    """
    K-means++ initialization for better starting centers.
    """
    n_samples = X.shape[0]

    centers = [X[rng.integers(0, n_samples)]]

    for _ in range(1, k):
        dist_to_nearest = np.min(
            ((X[:, None, :] - np.array(centers)[None, :, :]) ** 2).sum(axis=2),
            axis=1
        )

        total = np.sum(dist_to_nearest)

        if total == 0:
            centers.append(X[rng.integers(0, n_samples)])
            continue

        probs = dist_to_nearest / total
        cumulative_probs = np.cumsum(probs)

        r = rng.random()
        next_center_idx = np.searchsorted(cumulative_probs, r)
        next_center_idx = min(next_center_idx, n_samples - 1)

        centers.append(X[next_center_idx])

    return np.array(centers)


def kmeans_segmentation(image, k=4, max_iter=40, seed=0, include_xy=False):
    """
    K-Means segmentation.

    Output visualization:
    - Darkest cluster is colored red.
    - Other clusters are colored green.
    """
    arr = np.asarray(image)

    if arr.ndim == 2:
        arr_rgb = np.repeat(arr[..., None], 3, axis=2)
    else:
        arr_rgb = arr[..., :3]

    h, w = arr_rgb.shape[:2]

    X = _as_features(arr_rgb, include_xy=include_xy)
    rng = np.random.default_rng(seed)

    k = max(2, int(k))
    k = min(k, X.shape[0])

    centers = _kmeans_plusplus_init(X, k, rng)

    labels = np.zeros(X.shape[0], dtype=np.int32)

    for _ in range(max_iter):
        dist = ((X[:, None, :] - centers[None, :, :]) ** 2).sum(axis=2)
        new_labels = dist.argmin(axis=1)

        new_centers = centers.copy()

        for i in range(k):
            pts = X[new_labels == i]

            if len(pts):
                new_centers[i] = pts.mean(axis=0)
            else:
                new_centers[i] = X[rng.integers(0, X.shape[0])]

        center_shift = np.sqrt(((new_centers - centers) ** 2).sum())

        labels = new_labels
        centers = new_centers

        if center_shift < 1e-4:
            break

    labels_2d = labels.reshape((h, w))

    out = red_green_from_labels(arr_rgb, labels_2d)

    return out, labels_2d


def region_growing(image, seed_point=None, threshold=12, connectivity=8):
    """
    Region growing segmentation.

    Output visualization:
    - Grown region is red.
    - Background is green.

    seed_point should be:
    - None, or
    - (x, y)
    """
    arr = np.asarray(image)

    if arr.ndim == 2:
        gray = arr.astype(np.float32)
    else:
        gray = np.mean(arr[..., :3], axis=2).astype(np.float32)

    h, w = gray.shape

    if seed_point is None:
        _, otsu_mask = otsu_threshold(gray.astype(np.uint8))
        ys, xs = np.where(otsu_mask > 0)

        if len(xs) > 0:
            seed_point = (int(xs.mean()), int(ys.mean()))
        else:
            seed_point = (w // 2, h // 2)

    sx, sy = seed_point

    sx = int(np.clip(sx, 0, w - 1))
    sy = int(np.clip(sy, 0, h - 1))

    seed_value = gray[sy, sx]

    visited = np.zeros((h, w), dtype=bool)
    mask = np.zeros((h, w), dtype=np.uint8)

    q = deque([(sy, sx)])
    visited[sy, sx] = True

    nbr4 = [
        (-1, 0),
        (1, 0),
        (0, -1),
        (0, 1),
    ]

    nbr8 = nbr4 + [
        (-1, -1),
        (-1, 1),
        (1, -1),
        (1, 1),
    ]

    nbrs = nbr8 if connectivity == 8 else nbr4

    adaptive_threshold = max(float(threshold), np.std(gray) * 0.8)

    while q:
        y, x = q.popleft()

        if abs(float(gray[y, x]) - float(seed_value)) <= adaptive_threshold:
            mask[y, x] = 255

            for dy, dx in nbrs:
                ny = y + dy
                nx = x + dx

                if 0 <= ny < h and 0 <= nx < w and not visited[ny, nx]:
                    visited[ny, nx] = True
                    q.append((ny, nx))

    mask = _morphological_close(mask, iterations=1)

    out = red_green_from_mask(mask)

    return out


def _morphological_close(mask, iterations=1):
    """
    Simple closing operation:
    dilation followed by erosion.

    Implemented from scratch without OpenCV.
    """
    mask = mask.copy()

    nbr8 = [
        (-1, -1),
        (-1, 0),
        (-1, 1),
        (0, -1),
        (0, 1),
        (1, -1),
        (1, 0),
        (1, 1),
    ]

    h, w = mask.shape

    # Dilation
    dilated = mask.copy()

    for _ in range(iterations):
        new_mask = dilated.copy()

        ys, xs = np.where(dilated > 0)

        for y, x in zip(ys, xs):
            for dy, dx in nbr8:
                ny = y + dy
                nx = x + dx

                if 0 <= ny < h and 0 <= nx < w:
                    new_mask[ny, nx] = 255

        dilated = new_mask

    # Erosion
    eroded = dilated.copy()

    for _ in range(iterations):
        new_mask = eroded.copy()

        for y in range(h):
            for x in range(w):
                if eroded[y, x] > 0:
                    for dy, dx in nbr8:
                        ny = y + dy
                        nx = x + dx

                        if 0 <= ny < h and 0 <= nx < w:
                            if eroded[ny, nx] == 0:
                                new_mask[y, x] = 0
                                break

        eroded = new_mask

    return eroded


def agglomerative_segmentation(image, clusters=4, sample_size=64):
    """
    Hierarchical agglomerative clustering.

    Output visualization:
    - Darkest cluster is red.
    - Other clusters are green.

    Note:
    Agglomerative clustering is slow from scratch, so the image is downsampled.
    """
    arr = np.asarray(image)

    if arr.ndim == 2:
        arr_rgb = np.repeat(arr[..., None], 3, axis=2)
    else:
        arr_rgb = arr[..., :3]

    h, w = arr_rgb.shape[:2]

    small = Image.fromarray(arr_rgb.astype(np.uint8))
    small.thumbnail((sample_size, sample_size), Image.Resampling.LANCZOS)

    small_arr = np.asarray(small)

    sh, sw = small_arr.shape[:2]

    X = _as_features(small_arr, include_xy=True, spatial_weight=0.1)

    n_samples = X.shape[0]

    clusters = max(2, int(clusters))
    clusters = min(clusters, n_samples)

    groups = {i: [i] for i in range(n_samples)}
    centroids = {i: X[i].copy() for i in range(n_samples)}

    while len(groups) > clusters:
        keys = list(groups.keys())

        C = np.stack([centroids[k] for k in keys])

        d = ((C[:, None, :] - C[None, :, :]) ** 2).sum(axis=2)
        np.fill_diagonal(d, np.inf)

        if np.isinf(d).all():
            break

        a, b = np.unravel_index(np.argmin(d), d.shape)

        ka = keys[a]
        kb = keys[b]

        groups[ka].extend(groups[kb])

        pts = X[groups[ka]]
        centroids[ka] = pts.mean(axis=0)

        del groups[kb]
        del centroids[kb]

    labels_small = np.zeros((sh * sw,), dtype=np.int32)

    for label, idxs in enumerate(groups.values()):
        for idx in idxs:
            labels_small[idx] = label

    labels_small = labels_small.reshape((sh, sw))

    label_img = Image.fromarray(labels_small.astype(np.uint8)).resize(
        (w, h),
        resample=Image.Resampling.NEAREST
    )

    labels = np.asarray(label_img, dtype=np.int32)

    out = red_green_from_labels(arr_rgb, labels)

    return out, labels


def mean_shift_segmentation(
    image,
    bandwidth=0.18,
    max_iter=6,
    sample_size=96,
    merge_radius=None
):
    """
    Mean Shift segmentation.

    Output visualization:
    - Darkest cluster is red.
    - Other clusters are green.

    Note:
    Mean Shift is expensive from scratch, so the image is downsampled.
    """
    arr = np.asarray(image)

    if arr.ndim == 2:
        arr_rgb = np.repeat(arr[..., None], 3, axis=2)
    else:
        arr_rgb = arr[..., :3]

    h, w = arr_rgb.shape[:2]

    small = Image.fromarray(arr_rgb.astype(np.uint8))
    small.thumbnail((sample_size, sample_size), Image.Resampling.LANCZOS)

    small_arr = np.asarray(small)

    sh, sw = small_arr.shape[:2]

    X = _as_features(small_arr, include_xy=True, spatial_weight=0.15)

    n_samples = X.shape[0]

    bandwidth = float(bandwidth)

    if bandwidth <= 0:
        bandwidth = 0.18

    if bandwidth < 0.1:
        X_std = X.std(axis=0)
        bandwidth = 1.06 * X_std.mean() * (n_samples ** (-1.0 / 6.0))

    modes = X.copy()
    prev_modes = modes.copy()

    bw2 = bandwidth * bandwidth

    for _ in range(max_iter):
        for i in range(modes.shape[0]):
            dist2 = ((X - modes[i]) ** 2).sum(axis=1)
            neighbors = X[dist2 <= bw2]

            if len(neighbors) > 0:
                modes[i] = neighbors.mean(axis=0)

        shift = np.sqrt(((modes - prev_modes) ** 2).sum(axis=1)).mean()

        if shift < 1e-5:
            break

        prev_modes = modes.copy()

    if merge_radius is None:
        merge_radius = bandwidth / 2.0

    labels = np.zeros(n_samples, dtype=np.int32)
    assigned = np.zeros(n_samples, dtype=bool)

    cluster_id = 0

    for i in range(n_samples):
        if not assigned[i]:
            dist_to_mode = np.sqrt(((modes - modes[i]) ** 2).sum(axis=1))
            cluster_mask = dist_to_mode <= merge_radius

            labels[cluster_mask] = cluster_id
            assigned[cluster_mask] = True

            cluster_id += 1

    labels_small = labels.reshape((sh, sw))

    label_img = Image.fromarray(labels_small.astype(np.uint8)).resize(
        (w, h),
        resample=Image.Resampling.NEAREST
    )

    labels_full = np.asarray(label_img, dtype=np.int32)

    out = red_green_from_labels(arr_rgb, labels_full)

    return out, labels_full