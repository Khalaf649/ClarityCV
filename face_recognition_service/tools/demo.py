"""
Run face recognition on a single image from the command line.

    python -m tools.demo --model trained_model.pkl --image face.jpg
                         [--threshold 5] [--out result.png]

(or just ``python tools/demo.py ...`` from the service root.)

Prints results to stdout and saves an annotated image alongside.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

# Allow running as a plain script (`python tools/demo.py`) by putting the
# service root on the path before importing `core.*`.
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

import cv2  # noqa: E402

from core.pipeline import FaceRecognitionPipeline  # noqa: E402
from core.recognizer import EigenfaceRecognizer  # noqa: E402


def main() -> int:
    p = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter
    )
    p.add_argument("--model", required=True)
    p.add_argument("--image", required=True)
    p.add_argument("--threshold", type=float, default=5.0, help="UI slider value 1..10")
    p.add_argument("--out", default="result.png")
    args = p.parse_args()

    img = cv2.imread(args.image, cv2.IMREAD_COLOR)
    if img is None:
        print(f"ERROR: couldn't read {args.image}", file=sys.stderr)
        return 1

    print(f"Loading model from {args.model}...")
    recognizer = EigenfaceRecognizer.from_pickle(args.model)
    print(
        f"  {recognizer.n_components} components, "
        f"{recognizer.n_train} training projections, "
        f"image_shape={recognizer.image_shape}"
    )

    pipeline = FaceRecognitionPipeline(recognizer=recognizer)

    annotated, results, elapsed = pipeline.run(img, threshold=args.threshold)

    print()
    print(f"Detected {len(results)} face(s) in {elapsed:.3f}s:")
    for i, r in enumerate(results, 1):
        b = r.box
        rec = r.recognition
        flag = "✓" if rec.is_match else "✗"
        print(
            f"  [{i}] {flag} {rec.label:<20s}  "
            f"conf={rec.confidence:.3f}  dist={rec.distance:.3f}  "
            f"box=({b.x},{b.y},{b.w},{b.h})"
        )

    out_path = Path(args.out)
    cv2.imwrite(str(out_path), annotated)
    print()
    print(f"Saved annotated output → {out_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
