"""
Evaluate face recognition performance and plot the ROC curve.

Expected dataset layout (one folder per identity — the standard for ORL,
Yale, AT&T, LFW etc.):

    test_dir/
        person_a/
            img1.png
            img2.png
        person_b/
            ...

Usage:
    python evaluate.py --model trained_model.pkl --data path/to/test_dir
                       [--out report] [--no-detect]

Outputs (next to the chosen --out prefix):
    <out>_roc.png          ROC curve
    <out>_confusion.png    Confusion matrix
    <out>_metrics.txt      Accuracy, precision, recall, AUC, EER

The ROC is computed from genuine vs. impostor pair distances:
    • Genuine pair  = two probe-template projections with the same label
    • Impostor pair = two projections with different labels
A pair is "accepted" when its distance ≤ threshold; we sweep all
thresholds and plot TPR vs FPR.
"""
from __future__ import annotations

import argparse
import sys
from collections import defaultdict
from pathlib import Path
from typing import List, Tuple

# Allow running as a plain script (`python tools/evaluate.py`) by putting the
# service root on the path before importing `core.*`.
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import cv2
import numpy as np

from core.detector import FaceDetector, preprocess_face
from core.recognizer import EigenfaceRecognizer

IMG_EXTENSIONS = {".png", ".jpg", ".jpeg", ".bmp", ".pgm", ".tif", ".tiff"}


# ---------------------------------------------------------------------------
# Dataset loading
# ---------------------------------------------------------------------------

def load_dataset(data_dir: Path) -> List[Tuple[Path, str]]:
    samples: List[Tuple[Path, str]] = []
    for person_dir in sorted(p for p in data_dir.iterdir() if p.is_dir()):
        for img_path in sorted(person_dir.iterdir()):
            if img_path.suffix.lower() in IMG_EXTENSIONS:
                samples.append((img_path, person_dir.name))
    if not samples:
        raise SystemExit(
            f"No images found under {data_dir}. Expected one subfolder per identity."
        )
    return samples


def project_samples(
    samples: List[Tuple[Path, str]],
    recognizer: EigenfaceRecognizer,
    detect_faces: bool = True,
) -> Tuple[np.ndarray, List[str]]:
    detector = FaceDetector() if detect_faces else None
    target = recognizer.image_shape

    feats: List[np.ndarray] = []
    labels: List[str] = []
    skipped = 0

    for img_path, label in samples:
        img = cv2.imread(str(img_path), cv2.IMREAD_GRAYSCALE)
        if img is None:
            skipped += 1
            continue

        if detector is not None:
            boxes = detector.detect(img)
            if not boxes:
                skipped += 1
                continue
            # Use the largest detected face.
            box = max(boxes, key=lambda b: b.w * b.h)
            face = box.crop(img)
        else:
            face = img

        face = preprocess_face(face, target)
        feats.append(recognizer.project(face))
        labels.append(label)

    if skipped:
        print(f"  (skipped {skipped} images that couldn't be read or had no face)")
    if not feats:
        raise SystemExit("No usable samples — every image was skipped.")
    return np.vstack(feats), labels


# ---------------------------------------------------------------------------
# Identification metrics (rank-1)
# ---------------------------------------------------------------------------

def rank1_identification(
    probe_feats: np.ndarray,
    probe_labels: List[str],
    gallery_feats: np.ndarray,
    gallery_labels: List[str],
) -> Tuple[float, np.ndarray, List[str]]:
    correct = 0
    predictions: List[str] = []
    label_set = sorted(set(gallery_labels) | set(probe_labels))
    label_to_idx = {lab: i for i, lab in enumerate(label_set)}
    confusion = np.zeros((len(label_set), len(label_set)), dtype=int)

    for feat, true_label in zip(probe_feats, probe_labels):
        dists = np.linalg.norm(gallery_feats - feat, axis=1)
        idx = int(np.argmin(dists))
        pred = gallery_labels[idx]
        predictions.append(pred)
        if pred == true_label:
            correct += 1
        confusion[label_to_idx[true_label], label_to_idx[pred]] += 1

    accuracy = correct / len(probe_labels) if probe_labels else 0.0
    return accuracy, confusion, label_set


# ---------------------------------------------------------------------------
# Verification metrics — ROC
# ---------------------------------------------------------------------------

def build_pairs(
    feats: np.ndarray,
    labels: List[str],
    max_impostor_per_genuine: int = 5,
) -> Tuple[np.ndarray, np.ndarray]:
    """All genuine pairs and a roughly balanced sample of impostor pairs."""
    by_label: dict[str, List[int]] = defaultdict(list)
    for i, lab in enumerate(labels):
        by_label[lab].append(i)

    genuine: List[float] = []
    impostor: List[float] = []
    rng = np.random.default_rng(0)

    for indices in by_label.values():
        for i in range(len(indices)):
            for j in range(i + 1, len(indices)):
                genuine.append(float(np.linalg.norm(feats[indices[i]] - feats[indices[j]])))

    target_impostors = max(len(genuine), 100) * max_impostor_per_genuine
    n = len(labels)
    attempts = 0
    while len(impostor) < target_impostors and attempts < target_impostors * 10:
        i, j = rng.integers(0, n, size=2)
        if i != j and labels[i] != labels[j]:
            impostor.append(float(np.linalg.norm(feats[i] - feats[j])))
        attempts += 1

    return np.asarray(genuine), np.asarray(impostor)


def roc_from_pairs(
    genuine: np.ndarray,
    impostor: np.ndarray,
) -> Tuple[np.ndarray, np.ndarray, np.ndarray, float, float]:
    """Returns (thresholds, TPR, FPR, AUC, EER).

    A pair is "accepted" when distance ≤ threshold, so:
      TPR = fraction of genuine pairs accepted
      FPR = fraction of impostor pairs accepted
    """
    all_d = np.concatenate([genuine, impostor])
    thresholds = np.unique(all_d)
    # Add a few helper points so the curve starts at (0,0) and ends at (1,1).
    thresholds = np.concatenate(
        [[all_d.min() - 1e-6], thresholds, [all_d.max() + 1e-6]]
    )

    tpr = np.array([(genuine <= t).mean() for t in thresholds])
    fpr = np.array([(impostor <= t).mean() for t in thresholds])

    # Sort by FPR for AUC (trapezoid). NumPy renamed trapz → trapezoid in 2.x.
    order = np.argsort(fpr)
    trapezoid = getattr(np, "trapezoid", None) or getattr(np, "trapz")
    auc = float(trapezoid(tpr[order], fpr[order]))

    # Equal Error Rate — where FPR ≈ FNR (1 - TPR).
    fnr = 1.0 - tpr
    diff = np.abs(fpr - fnr)
    eer_idx = int(np.argmin(diff))
    eer = float((fpr[eer_idx] + fnr[eer_idx]) / 2.0)

    return thresholds, tpr, fpr, auc, eer


# ---------------------------------------------------------------------------
# Plotting
# ---------------------------------------------------------------------------

def plot_roc(fpr: np.ndarray, tpr: np.ndarray, auc: float, eer: float, out_path: Path) -> None:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    fig, ax = plt.subplots(figsize=(6, 6))
    order = np.argsort(fpr)
    ax.plot(fpr[order], tpr[order], lw=2, label=f"AUC = {auc:.3f}")
    ax.plot([0, 1], [0, 1], "k--", lw=1, alpha=0.5, label="chance")
    ax.scatter([eer], [1 - eer], color="red", zorder=5, label=f"EER = {eer:.3f}")
    ax.set_xlabel("False Positive Rate")
    ax.set_ylabel("True Positive Rate")
    ax.set_title("Face Recognition ROC")
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1.02)
    ax.grid(alpha=0.3)
    ax.legend(loc="lower right")
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def plot_confusion(confusion: np.ndarray, labels: List[str], out_path: Path) -> None:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    n = len(labels)
    fig, ax = plt.subplots(figsize=(max(6, n * 0.4), max(5, n * 0.4)))
    im = ax.imshow(confusion, cmap="Blues", aspect="auto")
    ax.set_title("Confusion matrix (rank-1 identification)")
    if n <= 30:
        ax.set_xticks(range(n))
        ax.set_yticks(range(n))
        ax.set_xticklabels(labels, rotation=90, fontsize=7)
        ax.set_yticklabels(labels, fontsize=7)
    ax.set_xlabel("predicted")
    ax.set_ylabel("true")
    fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    p.add_argument("--model", required=True, help="Path to trained_model.pkl")
    p.add_argument("--data", required=True, help="Test dataset root (one folder per identity)")
    p.add_argument("--out", default="report", help="Output filename prefix")
    p.add_argument(
        "--no-detect",
        action="store_true",
        help="Skip face detection (use the whole image — for already-cropped datasets)",
    )
    args = p.parse_args()

    print(f"Loading model: {args.model}")
    recognizer = EigenfaceRecognizer.from_pickle(args.model)
    print(
        f"  components={recognizer.n_components} "
        f"train_samples={recognizer.n_train} "
        f"image_shape={recognizer.image_shape}"
    )

    print(f"Loading dataset: {args.data}")
    samples = load_dataset(Path(args.data))
    print(f"  {len(samples)} test image(s) across {len(set(s[1] for s in samples))} identities")

    print("Projecting samples...")
    feats, labels = project_samples(
        samples, recognizer, detect_faces=not args.no_detect
    )

    # Identification (rank-1) — gallery is the recognizer's own training set.
    print("Computing rank-1 identification accuracy...")
    accuracy, confusion, label_list = rank1_identification(
        feats,
        labels,
        recognizer.projections,
        [str(l) for l in recognizer.labels],
    )

    print("Computing verification ROC...")
    genuine, impostor = build_pairs(feats, labels)
    thresholds, tpr, fpr, auc, eer = roc_from_pairs(genuine, impostor)

    out_prefix = Path(args.out)
    out_prefix.parent.mkdir(parents=True, exist_ok=True)

    roc_path = out_prefix.with_name(out_prefix.name + "_roc.png")
    confusion_path = out_prefix.with_name(out_prefix.name + "_confusion.png")
    metrics_path = out_prefix.with_name(out_prefix.name + "_metrics.txt")

    plot_roc(fpr, tpr, auc, eer, roc_path)
    plot_confusion(confusion, label_list, confusion_path)

    with open(metrics_path, "w") as f:
        f.write("Face Recognition Performance Report\n")
        f.write("=" * 40 + "\n\n")
        f.write(f"Test images:           {len(labels)}\n")
        f.write(f"Identities (probe):    {len(set(labels))}\n")
        f.write(f"Identities (gallery):  {len(recognizer.unique_labels)}\n")
        f.write(f"Genuine pairs:         {len(genuine)}\n")
        f.write(f"Impostor pairs:        {len(impostor)}\n\n")
        f.write(f"Rank-1 accuracy:       {accuracy:.4f}\n")
        f.write(f"ROC AUC:               {auc:.4f}\n")
        f.write(f"Equal Error Rate:      {eer:.4f}\n\n")
        f.write(f"Genuine distance:      mean={genuine.mean():.3f}  std={genuine.std():.3f}\n")
        f.write(f"Impostor distance:     mean={impostor.mean():.3f}  std={impostor.std():.3f}\n")

    print()
    print(f"Rank-1 accuracy:  {accuracy:.4f}")
    print(f"ROC AUC:          {auc:.4f}")
    print(f"EER:              {eer:.4f}")
    print()
    print(f"Wrote: {metrics_path}")
    print(f"Wrote: {roc_path}")
    print(f"Wrote: {confusion_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
