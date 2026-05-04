# No-OpenCV Image Segmenter — FastAPI Backend

This is the same project logic as the original no-OpenCV implementation, but exposed as a FastAPI backend so you can call it from a React frontend.

It implements the required image processing task from scratch using NumPy and Pillow only. It does **not** use OpenCV / `cv2`.

## Implemented methods

### Grayscale thresholding

- Optimal thresholding
- Otsu thresholding
- Spectral / multi-level thresholding for more than two modes
- Local adaptive thresholding

### Grayscale or color segmentation

- K-means
- Region growing
- Agglomerative clustering
- Mean shift

## Project structure

```text
fastapi_image_segmenter/
├── app/
│   ├── main.py              # FastAPI routes
│   ├── image_service.py     # Connects API requests to algorithms
│   └── schemas.py           # Pydantic response models
├── algorithms/
│   ├── thresholding.py      # Original thresholding logic
│   └── segmentation.py      # Original segmentation logic
├── utils/
│   ├── image_io.py          # Pillow/NumPy image helpers
│   └── palette.py
├── frontend_examples/
│   ├── api.js               # React API helper
│   └── ImageSegmenter.jsx   # Example React component
├── run_cli.py               # Optional CLI runner
├── demo_generate_outputs.py # Optional demo generator
└── requirements.txt
```

## 1. Run the FastAPI backend

From inside this project folder:

```bash
python -m venv .venv
```

Activate it:

```bash
# Windows PowerShell
.venv\Scripts\Activate.ps1

# macOS/Linux
source .venv/bin/activate
```

Install dependencies:

```bash
pip install -r requirements.txt
```

Start the backend:

```bash
uvicorn app.main:app --reload --host 0.0.0.0 --port 8000
```

Open the interactive API docs:

```text
http://localhost:8000/docs
```

Health check:

```text
http://localhost:8000/api/health
```

## 2. API endpoints

### `GET /api/methods`

Returns the supported categories, methods, and default parameters.

### `POST /api/segment`

Send a `multipart/form-data` request with:

| Field | Required | Example |
|---|---:|---|
| `file` | yes | image file |
| `category` | yes | `thresholding` or `segmentation` |
| `method` | yes | `otsu`, `kmeans`, `mean_shift`, etc. |
| method params | no | `levels=3`, `k=4`, `threshold=12` |

The response is JSON:

```json
{
  "method": "otsu",
  "filename": "brain.png",
  "mime_type": "image/png",
  "image_base64": "...",
  "metadata": {
    "threshold": 128
  }
}
```

Use the result image in React like this:

```js
const imageUrl = `data:${data.mime_type};base64,${data.image_base64}`;
```

## 3. Curl examples

Otsu thresholding:

```bash
curl -X POST "http://localhost:8000/api/segment" \
  -F "file=@samples/your_image.png" \
  -F "category=thresholding" \
  -F "method=otsu"
```

Spectral thresholding with three classes:

```bash
curl -X POST "http://localhost:8000/api/segment" \
  -F "file=@samples/your_image.png" \
  -F "category=thresholding" \
  -F "method=spectral" \
  -F "levels=3"
```

K-means segmentation:

```bash
curl -X POST "http://localhost:8000/api/segment" \
  -F "file=@samples/your_image.png" \
  -F "category=segmentation" \
  -F "method=kmeans" \
  -F "k=4"
```

Mean shift segmentation:

```bash
curl -X POST "http://localhost:8000/api/segment" \
  -F "file=@samples/your_image.png" \
  -F "category=segmentation" \
  -F "method=mean_shift" \
  -F "bandwidth=0.18" \
  -F "sample_size=48"
```

## 4. Connect with your React frontend

### Vite React

Create `.env` in your React project:

```env
VITE_API_BASE_URL=http://localhost:8000
```

Copy `frontend_examples/api.js` into your React `src` folder, for example:

```text
src/api/imageSegmentationApi.js
```

Then use it:

```js
import { segmentImage } from "./api/imageSegmentationApi";

const data = await segmentImage({
  file,
  category: "thresholding",
  method: "otsu",
});

setResultUrl(data.imageUrl);
```

### Create React App

Use this env name instead:

```env
REACT_APP_API_BASE_URL=http://localhost:8000
```

Then change the first line in `api.js` to:

```js
const API_BASE_URL = process.env.REACT_APP_API_BASE_URL || "http://localhost:8000";
```

## 5. Important CORS note

The backend currently allows these React development origins:

```text
http://localhost:3000
http://127.0.0.1:3000
http://localhost:5173
http://127.0.0.1:5173
```

If your frontend runs on another port, edit `allow_origins` in `app/main.py`.

## 6. No OpenCV rule

This project intentionally avoids:

```python
import cv2
```

All image loading, saving, and encoding is done with Pillow. All algorithm logic is implemented with NumPy and standard Python.
