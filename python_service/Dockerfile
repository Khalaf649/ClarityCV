# ---------------------------------------------------------------------------
# Face Recognition service (FastAPI + OpenCV + NumPy)
# ---------------------------------------------------------------------------
FROM python:3.11-slim AS runtime

ENV DEBIAN_FRONTEND=noninteractive \
    PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    PIP_NO_CACHE_DIR=1

# OpenCV needs a few system libs even for the headless wheel.
RUN apt-get update && apt-get install -y --no-install-recommends \
        libglib2.0-0 \
        libsm6 \
        libxext6 \
        libxrender1 \
        libgl1 \
        curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Install Python deps. Swap to opencv-python-headless so we don't pull X11.
COPY requirements.txt ./
RUN sed -i 's/opencv-python\b/opencv-python-headless/' requirements.txt \
    && pip install --no-cache-dir -r requirements.txt

# Copy source — core/, api/, tools/, plus the model and configs.
COPY . .

# Non-root user.
RUN useradd --system --create-home --shell /bin/bash appuser \
    && chown -R appuser:appuser /app
USER appuser

ENV PORT=8081 \
    MODEL_PATH=/app/trained_model.pkl

EXPOSE 8081

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD curl -f http://localhost:8081/api/health || exit 1

# Entry point — runs the FastAPI app via the package's __main__ block,
# which calls uvicorn.run("api.server:app", ...) using the PORT env var.
CMD ["python", "-m", "api.server"]
