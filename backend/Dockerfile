# ---------------------------------------------------------------------------
# Stage 1: Builder
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ca-certificates \
    wget \
    curl \
    libopencv-dev \
    libssl-dev \
    zlib1g-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Download cpp-httplib single header (much faster than FetchContent)
RUN wget -q -O /usr/local/include/httplib.h \
    https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.15.3/httplib.h

WORKDIR /build

COPY . .

RUN cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/install \
    && cmake --build build --parallel $(nproc) \
    && cmake --install build

# ---------------------------------------------------------------------------
# Stage 2: Runtime
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libopencv-dev \
    libssl3 \
    curl \
    && rm -rf /var/lib/apt/lists/*

RUN useradd --system --create-home --shell /bin/bash appuser

COPY --from=builder /install/bin/cv_backend /usr/local/bin/cv_backend

RUN chown appuser:appuser /usr/local/bin/cv_backend

USER appuser

ENV SERVER_HOST=0.0.0.0
ENV SERVER_PORT=8080
ENV SERVER_THREADS=4

EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD curl -f http://localhost:8080/api/health || exit 1

CMD ["cv_backend"]
