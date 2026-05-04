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
    xy = np.stack([yy / max(h - 1, 1), xx / max(w - 1, 1)], axis=-1).reshape(-1, 2)
    return np.concatenate([color, spatial_weight * xy], axis=1)


def _kmeans_plusplus_init(X, k, rng):
    """K-means++ initialization for better starting centers."""
    n_samples = X.shape[0]
    centers = [X[rng.integers(0, n_samples)]]
    
    for _ in range(1, k):
        dist_to_nearest = np.min(((X[:, None, :] - np.array(centers)[None, :, :]) ** 2).sum(axis=2), axis=1)
        probs = dist_to_nearest / np.sum(dist_to_nearest)
        cumulative_probs = np.cumsum(probs)
        r = rng.random()
        next_center_idx = np.searchsorted(cumulative_probs, r)
        centers.append(X[next_center_idx])
    
    return np.array(centers)


def kmeans_segmentation(image, k=4, max_iter=40, seed=0, include_xy=False):
    """Enhanced K-means with k-means++ initialization and green color palette."""
    arr = np.asarray(image)
    h, w = arr.shape[:2]
    X = _as_features(arr, include_xy=include_xy)
    rng = np.random.default_rng(seed)
    
    # Use k-means++ initialization instead of random
    centers = _kmeans_plusplus_init(X, k, rng)

    labels = np.zeros(X.shape[0], dtype=np.int32)
    prev_centers = centers.copy()
    
    for iteration in range(max_iter):
        dist = ((X[:, None, :] - centers[None, :, :]) ** 2).sum(axis=2)
        new_labels = dist.argmin(axis=1)
        new_centers = centers.copy()
        
        for i in range(k):
            pts = X[new_labels == i]
            if len(pts):
                new_centers[i] = pts.mean(axis=0)
            else:
                # If cluster is empty, reinitialize with a random point
                new_centers[i] = X[rng.integers(0, X.shape[0])]
        
        # Check for convergence with tolerance
        center_shift = np.sqrt(((new_centers - centers) ** 2).sum())
        if center_shift < 1e-4:
            centers = new_centers
            labels = new_labels
            break
        
        labels, centers = new_labels, new_centers

    # Generate distinct green color palette
    labels_2d = labels.reshape((h, w))
    out = np.zeros((h, w, 3), dtype=np.uint8)
    for i in range(k):
        # Create gradient from dark green to bright green
        hue_step = 255 // max(k, 1)
        green_val = 100 + (i * hue_step) % 155
        blue_val = max(0, 50 - i * 20)
        red_val = max(0, 80 - i * 30)
        mask = (labels_2d == i)
        out[mask] = [red_val, green_val, blue_val]
    
    return out, labels_2d


def region_growing(image, seed_point=None, threshold=12, connectivity=8):
    """Enhanced region growing with adaptive seed selection and morphological cleanup."""
    gray = image if np.asarray(image).ndim == 2 else np.mean(np.asarray(image)[..., :3], axis=2)
    gray = gray.astype(np.float32)
    h, w = gray.shape
    
    # Improved seed point selection
    if seed_point is None:
        # Use image gradient to find a good seed (center of largest region)
        _, mask = otsu_threshold(gray.astype(np.uint8))
        ys, xs = np.where(mask > 0)
        if len(xs) > 0:
            # Find center of mass for more stable seed
            seed_point = (int(xs.mean()), int(ys.mean()))
        else:
            seed_point = (w // 2, h // 2)
    
    sx, sy = seed_point
    sx, sy = int(np.clip(sx, 0, w - 1)), int(np.clip(sy, 0, h - 1))
    seed_value = gray[sy, sx]
    visited = np.zeros((h, w), dtype=bool)
    mask = np.zeros((h, w), dtype=np.uint8)
    q = deque([(sy, sx)])
    visited[sy, sx] = True
    nbr4 = [(-1, 0), (1, 0), (0, -1), (0, 1)]
    nbr8 = nbr4 + [(-1, -1), (-1, 1), (1, -1), (1, 1)]
    nbrs = nbr8 if connectivity == 8 else nbr4

    # Less aggressive: use higher threshold multiplier
    adaptive_threshold = max(threshold * 2, np.std(gray) * 1.5)

    while q:
        y, x = q.popleft()
        if abs(float(gray[y, x]) - float(seed_value)) <= adaptive_threshold:
            mask[y, x] = 255
            for dy, dx in nbrs:
                ny, nx = y + dy, x + dx
                if 0 <= ny < h and 0 <= nx < w and not visited[ny, nx]:
                    visited[ny, nx] = True
                    q.append((ny, nx))
    
    # Simple morphological closing: dilate then erode to fill small holes
    if mask.sum() > 0:
        # Dilation using 8-connectivity
        dilated = mask.copy()
        for _ in range(2):  # 2 iterations for mild closing
            new_mask = dilated.copy()
            for y in range(h):
                for x in range(w):
                    if dilated[y, x] > 0:
                        for dy, dx in nbr8:
                            ny, nx = y + dy, x + dx
                            if 0 <= ny < h and 0 <= nx < w:
                                new_mask[ny, nx] = 255
            dilated = new_mask
        
        # Erosion using 8-connectivity
        for _ in range(2):  # 2 iterations for mild erosion
            new_mask = dilated.copy()
            for y in range(h):
                for x in range(w):
                    if dilated[y, x] == 0:
                        for dy, dx in nbr8:
                            ny, nx = y + dy, x + dx
                            if 0 <= ny < h and 0 <= nx < w:
                                new_mask[ny, nx] = 0
            dilated = new_mask
        
        mask = dilated
    
    return mask


def agglomerative_segmentation(image, clusters=4, sample_size=32):
    """Hierarchical agglomerative clustering optimized for quality."""
    arr = np.asarray(image)
    if arr.ndim == 2:
        arr = np.repeat(arr[..., None], 3, axis=2)
    h, w = arr.shape[:2]
    
    # Downsample for clustering efficiency
    small = Image.fromarray(arr.astype(np.uint8))
    small.thumbnail((sample_size, sample_size), Image.Resampling.LANCZOS)
    small_arr = np.asarray(small)
    sh, sw = small_arr.shape[:2]
    
    # Extract features on downsampled image
    X = _as_features(small_arr, include_xy=True, spatial_weight=0.1)
    n_samples = X.shape[0]
    
    # Start with each pixel as its own cluster
    groups = {i: [i] for i in range(n_samples)}
    centroids = {i: X[i].copy() for i in range(n_samples)}
    
    # Merge clusters until target number reached (but limit iterations for speed)
    max_merges = max(n_samples - clusters, 1)
    merges_done = 0
    
    while len(groups) > clusters and merges_done < max_merges:
        keys = list(groups.keys())
        if len(keys) <= 1:
            break
            
        # Find closest pair of clusters
        C = np.stack([centroids[k] for k in keys])
        d = ((C[:, None, :] - C[None, :, :]) ** 2).sum(axis=2)
        np.fill_diagonal(d, np.inf)
        
        if np.isinf(d).all():
            break
            
        a, b = np.unravel_index(np.argmin(d), d.shape)
        ka, kb = keys[a], keys[b]
        
        # Merge clusters
        groups[ka].extend(groups[kb])
        pts = X[groups[ka]]
        centroids[ka] = pts.mean(axis=0)
        del groups[kb]
        del centroids[kb]
        merges_done += 1
    
    # Create label map from downsampled image
    labels_small = np.zeros((sh * sw,), dtype=np.int32)
    for label, idxs in enumerate(groups.values()):
        for idx in idxs:
            labels_small[idx] = label
    labels_small = labels_small.reshape((sh, sw))
    
    # Upsample to original size
    label_img = Image.fromarray(labels_small.astype(np.uint8)).resize((w, h), resample=Image.Resampling.NEAREST)
    labels = np.asarray(label_img, dtype=np.int32)
    
    # Color by original image content
    out = np.zeros((h, w, 3), dtype=np.uint8)
    for label_val in np.unique(labels):
        mask = (labels == label_val)
        if np.any(mask):
            out[mask] = arr[mask].mean(axis=0).astype(np.uint8)
    return out, labels


def mean_shift_segmentation(image, bandwidth=0.18, max_iter=6, sample_size=48, merge_radius=None):
    """Mean shift clustering with convergence stopping."""
    arr = np.asarray(image)
    if arr.ndim == 2:
        arr = np.repeat(arr[..., None], 3, axis=2)
    h, w = arr.shape[:2]
    
    # Downsample image for efficiency
    small = Image.fromarray(arr.astype(np.uint8))
    small.thumbnail((sample_size, sample_size), Image.Resampling.LANCZOS)
    small_arr = np.asarray(small)
    sh, sw = small_arr.shape[:2]
    X = _as_features(small_arr, include_xy=True, spatial_weight=0.15)
    n_samples = X.shape[0]
    
    # Estimate bandwidth from data if needed
    if bandwidth < 0.1:
        X_std = X.std(axis=0)
        bandwidth = 1.06 * X_std.mean() * (n_samples ** (-1.0 / 6.0))
    
    # Initialize modes as copy of data points
    modes = X.copy()
    prev_modes = modes.copy()
    bw2 = bandwidth * bandwidth
    
    # Mean shift iterations
    for iteration in range(max_iter):
        for i in range(modes.shape[0]):
            # Find neighbors within bandwidth
            dist2 = ((X - modes[i]) ** 2).sum(axis=1)
            neighbors = X[dist2 <= bw2]
            if len(neighbors) > 0:
                modes[i] = neighbors.mean(axis=0)
        
        # Check convergence
        shift = np.sqrt(((modes - prev_modes) ** 2).sum(axis=1)).mean()
        if shift < 1e-5:
            break
        prev_modes = modes.copy()
    
    # Merge nearby modes
    if merge_radius is None:
        merge_radius = bandwidth / 2.0
    
    labels = np.zeros(n_samples, dtype=np.int32)
    cluster_id = 0
    assigned = np.zeros(n_samples, dtype=bool)
    
    for i in range(n_samples):
        if not assigned[i]:
            dist_to_mode = np.sqrt(((modes - modes[i]) ** 2).sum(axis=1))
            cluster_mask = (dist_to_mode <= merge_radius)
            labels[cluster_mask] = cluster_id
            assigned[cluster_mask] = True
            cluster_id += 1
    
    labels_small = labels.reshape((sh, sw))
    
    # Upsample to original size
    label_img = Image.fromarray(labels_small.astype(np.uint8)).resize((w, h), resample=Image.Resampling.NEAREST)
    labels_full = np.asarray(label_img, dtype=np.int32)
    
    # Color by original image
    out = np.zeros((h, w, 3), dtype=np.uint8)
    for label_val in np.unique(labels_full):
        mask = (labels_full == label_val)
        if np.any(mask):
            out[mask] = arr[mask].mean(axis=0).astype(np.uint8)
    return out, labels_full
