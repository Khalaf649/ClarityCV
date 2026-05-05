# Face Recognition Service

PCA / Eigenfaces face recognition for ClarityCV.

This service consumes a `trained_model.pkl` produced by the training script
at the repo root (`train_and_save_model.py` / `classes/face_recognition.py`)
and provides:

1. **End-to-end pipeline** — Haar-cascade face detection → resize → project
   into the eigenface subspace → nearest-neighbour identification using the
   same `confidence = 1 − min_dist/max_dist` formula the training script
   uses, so the threshold you trained with (e.g. 0.85) applies directly here.
2. **FastAPI server** that exposes `POST /api/recognize_faces` — the
   endpoint the existing frontend (`frontend/app/face-recognition`) already
   calls — and `GET /api/health` for Docker / k8s probes.
3. **CLI tools** for inspecting models, demoing a single image, and
   computing rank-1 accuracy / AUC / EER + plotting the ROC curve
   (Task 4 of the assignment).

## Layout

```
face_recognition_service/
├── core/                    Pure algorithms — no HTTP, no CLI
│   ├── __init__.py
│   ├── recognizer.py        Eigenfaces classifier + tolerant .pkl loader
│   ├── detector.py          Haar cascade wrapper + face preprocessing
│   └── pipeline.py          detect → project → classify orchestrator
├── api/                     HTTP service
│   ├── __init__.py
│   ├── server.py            FastAPI app and routes
│   └── schemas.py           Pydantic request/response models
├── tools/                   CLI utilities
│   ├── __init__.py
│   ├── demo.py              Recognize a single image
│   ├── evaluate.py          ROC curve + accuracy/AUC/EER (Task 4)
│   └── inspect_model.py     Diagnostic — print every key in the .pkl
├── Dockerfile
├── .dockerignore
├── README.md
├── requirements.txt
└── trained_model.pkl        Bind/baked into the image at /app/trained_model.pkl
```

## Setup

```bash
cd face_recognition_service
python -m venv .venv
source .venv/bin/activate          # Windows: .venv\Scripts\activate
pip install -r requirements.txt
```

Drop your `trained_model.pkl` in this folder (or anywhere — pass `--model`
or set `MODEL_PATH`).

## 1. Verify the model loads

```bash
python -m tools.inspect_model trained_model.pkl
```

The loader recognises a wide range of common key names (see
`MODEL_KEY_ALIASES` in `core/recognizer.py`), but training scripts vary. If
`inspect_model` shows your file uses a key the loader doesn't know about,
add the name to the alias list and you're done. The shipped training script
saves under `mean_face` / `eigenfaces` / `dataset_projections` /
`train_labels` / `threshold` — all of which are recognised out of the box.
The image shape is inferred from `train_images[0].shape` if not stored.

## 2. Try it on one image

```bash
python -m tools.demo --model trained_model.pkl --image some_face.jpg --out result.png
```

Output looks like:

```
Loading model from trained_model.pkl...
  10 components, 500 training projections, image_shape=(64, 64)

Detected 1 face(s) in 0.043s:
  [1] ✓ s17                   conf=1.000  dist=0.0   box=(0,0,64,64)

Saved annotated output → result.png
```

## 3. Run as a service (wires up to the web UI)

```bash
MODEL_PATH=trained_model.pkl python -m api.server
# → http://localhost:8081
```

Or with uvicorn directly:

```bash
uvicorn api.server:app --host 0.0.0.0 --port 8081 --reload
```

The server listens on **8081** by default so it doesn't collide with the
existing C++ backend on 8080. The frontend has already been wired up
(`frontend/next.config.ts`) to forward `/api/recognize_faces` here while
everything else keeps going to the C++ backend.

### Threshold semantics

The UI slider is 1..10. The recognizer maps it linearly onto a confidence
cutoff in 0..1, so the value the user trained with (e.g. 0.85) corresponds
to a slider position of **8.5**:

* slider = 8.5 → confidence cutoff 0.85 (the trained default)
* slider < 8.5 → looser (more permissive — risk of false-accepts)
* slider > 8.5 → stricter (more "Unknown" labels — risk of false-rejects)

Faces below the cutoff get labelled "Unknown" and drawn in red.

## 4. Performance + ROC (Task 4 of the assignment)

Arrange your test set as one folder per identity:

```
testset/
  s1/  img01.png img02.png ...
  s2/  ...
  ...
```

Then run:

```bash
python -m tools.evaluate --model trained_model.pkl --data testset --out results/run1
```

If your test images are already tightly cropped faces (ORL / AT&T / Yale /
GTdb style), skip the detection step:

```bash
python -m tools.evaluate --model trained_model.pkl --data testset --no-detect \
                          --out results/run1
```

You'll get three files:

* `results/run1_roc.png` — ROC curve with AUC and EER marked
* `results/run1_confusion.png` — confusion matrix
* `results/run1_metrics.txt` — accuracy, AUC, EER, pair distance stats

The ROC sweeps every distance threshold and plots TPR vs FPR over genuine
and impostor projection pairs. AUC and EER are reported in the text file.

## Notes

* Detection uses OpenCV's bundled Haar cascade
  (`haarcascade_frontalface_default.xml`) — no extra downloads. On
  pre-cropped face datasets where Haar misses tiny portraits, the detector
  falls back to "treat the whole image as one face" so recognition still
  works.
* Faces are resized to the model's `image_shape` and converted to grayscale
  before projection — **no histogram equalization by default** because the
  shipped training script (`classes/face_recognition.py`) doesn't equalize
  either, and the inference preprocessing must match training. If your
  training script *does* equalize, pass `equalize=True` to
  `core.detector.preprocess_face` so accuracy doesn't drop.
* Uploaded images are downscaled to a max of 1500 px on the longest side
  before processing (configurable via `MAX_INPUT_DIM` in `api/server.py`).
  This bounds Haar detection latency on phone-sized photos — a 4 MP image
  drops from ~8 s to ~1.7 s with no loss of recognition accuracy because
  the model only ever sees the cropped face at `image_shape`.
* The C++ backend on port 8080 is left untouched — this service only owns
  `/api/recognize_faces`.

## Docker

A `Dockerfile` is included and the top-level `docker-compose.yml` already
wires the service in alongside the C++ backend and the frontend. Place
`trained_model.pkl` in this directory **before** building, then from the
repo root:

```bash
docker compose up --build
```

This brings up three containers on a shared network:

| service            | container             | host port |
| ------------------ | --------------------- | --------- |
| backend (C++)      | `cv_backend`          | 8080      |
| face_recognition   | `cv_face_recognition` | 8081      |
| frontend (Next.js) | `cv_frontend`         | 3000      |

The `.pkl` is baked into the image at build time. To swap models, drop in
the new file and rebuild just this service:

```bash
docker compose build face_recognition && docker compose up -d face_recognition
```

If you'd rather hot-swap without rebuilding, uncomment the `volumes:` block
in `docker-compose.yml` — but the file has to exist on the host *before*
you run `docker compose up`, or Docker creates an empty directory in its
place.

Inside the compose network the frontend reaches the service at
`http://face_recognition:8081`; from your host it's `http://localhost:8081`.
