import numpy as np


def labels_to_random_colors(labels, seed=7):
    labels = np.asarray(labels)
    unique = np.unique(labels)
    rng = np.random.default_rng(seed)
    colors = rng.integers(0, 256, size=(max(len(unique), 1), 3), dtype=np.uint8)
    mapping = {label: colors[i] for i, label in enumerate(unique)}
    out = np.zeros(labels.shape + (3,), dtype=np.uint8)
    for label, color in mapping.items():
        out[labels == label] = color
    return out


def labels_to_gray(labels):
    labels = np.asarray(labels)
    unique = np.unique(labels)
    if len(unique) <= 1:
        return np.zeros(labels.shape, dtype=np.uint8)
    rank = {label: i for i, label in enumerate(unique)}
    out = np.zeros(labels.shape, dtype=np.float32)
    for label, i in rank.items():
        out[labels == label] = i
    return (out * 255 / (len(unique) - 1)).astype(np.uint8)
