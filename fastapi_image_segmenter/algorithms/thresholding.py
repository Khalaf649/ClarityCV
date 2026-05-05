import numpy as np


def binary_from_threshold(gray, threshold):
    return (gray > threshold).astype(np.uint8) * 255


def optimal_threshold(gray, epsilon=0.5, max_iter=100):
    """Iterative global thresholding using background/corner initialization."""
    gray = gray.astype(np.float32)
    corners = np.array([gray[0, 0], gray[0, -1], gray[-1, 0], gray[-1, -1]], dtype=np.float32)
    background_mean = float(corners.mean())
    object_pixels = gray[np.abs(gray - background_mean) > 1e-6]
    object_mean = float(object_pixels.mean()) if object_pixels.size else float(gray.mean())
    t = (background_mean + object_mean) / 2.0

    for _ in range(max_iter):
        g1 = gray[gray <= t]
        g2 = gray[gray > t]
        if g1.size == 0 or g2.size == 0:
            break
        new_t = (float(g1.mean()) + float(g2.mean())) / 2.0
        if abs(new_t - t) < epsilon:
            t = new_t
            break
        t = new_t
    return t, binary_from_threshold(gray, t)


def otsu_threshold(gray):
    """Classic Otsu thresholding implemented from the histogram."""
    gray = gray.astype(np.uint8)
    hist = np.bincount(gray.ravel(), minlength=256).astype(np.float64)
    prob = hist / hist.sum()
    omega = np.cumsum(prob)
    mu = np.cumsum(prob * np.arange(256))
    mu_t = mu[-1]
    denom = omega * (1.0 - omega)
    denom[denom == 0] = np.nan
    sigma_b = (mu_t * omega - mu) ** 2 / denom
    t = int(np.nanargmax(sigma_b))
    return t, binary_from_threshold(gray, t)


def _class_variance(prefix_p, prefix_m, a, b):
    # inclusive range [a, b]
    if a > b:
        return 0.0
    w = prefix_p[b + 1] - prefix_p[a]
    if w <= 0:
        return 0.0
    m = prefix_m[b + 1] - prefix_m[a]
    return (m * m) / w


def spectral_threshold(gray, levels=3):
    """Multi-level Otsu/spectral thresholding for several intensity modes.

    `levels` means number of output classes. For more than two modes, use 3 or more.
    Dynamic programming maximizes between-class variance.
    """
    levels = int(max(2, min(levels, 8)))
    gray = gray.astype(np.uint8)
    hist = np.bincount(gray.ravel(), minlength=256).astype(np.float64)
    prob = hist / max(hist.sum(), 1)
    values = np.arange(256, dtype=np.float64)
    prefix_p = np.concatenate([[0.0], np.cumsum(prob)])
    prefix_m = np.concatenate([[0.0], np.cumsum(prob * values)])

    dp = np.full((levels + 1, 256), -np.inf)
    parent = np.zeros((levels + 1, 256), dtype=np.int16)

    for i in range(256):
        dp[1, i] = _class_variance(prefix_p, prefix_m, 0, i)

    for k in range(2, levels + 1):
        for i in range(k - 1, 256):
            best_val = -np.inf
            best_j = k - 2
            for j in range(k - 2, i):
                val = dp[k - 1, j] + _class_variance(prefix_p, prefix_m, j + 1, i)
                if val > best_val:
                    best_val = val
                    best_j = j
            dp[k, i] = best_val
            parent[k, i] = best_j

    thresholds = []
    idx = 255
    for k in range(levels, 1, -1):
        j = int(parent[k, idx])
        thresholds.append(j)
        idx = j
    thresholds = sorted(thresholds)

    labels = np.digitize(gray, thresholds, right=True).astype(np.uint8)
    output = (labels * (255 // max(levels - 1, 1))).astype(np.uint8)
    return thresholds, output


def local_threshold(gray, window_size=31, offset=5):
    """Local adaptive mean thresholding using an integral image."""
    gray = gray.astype(np.float32)
    window_size = int(window_size)
    if window_size % 2 == 0:
        window_size += 1
    radius = window_size // 2
    padded = np.pad(gray, radius, mode="reflect")
    integral = padded.cumsum(axis=0).cumsum(axis=1)
    h, w = gray.shape
    local_mean = np.zeros_like(gray)

    for y in range(h):
        y1, y2 = y, y + window_size - 1
        for x in range(w):
            x1, x2 = x, x + window_size - 1
            total = integral[y2, x2]
            if y1 > 0:
                total -= integral[y1 - 1, x2]
            if x1 > 0:
                total -= integral[y2, x1 - 1]
            if y1 > 0 and x1 > 0:
                total += integral[y1 - 1, x1 - 1]
            local_mean[y, x] = total / (window_size * window_size)
    return ((gray > (local_mean - offset)).astype(np.uint8) * 255)
