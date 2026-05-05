# ---------------------------------------------------------------------------
# Stage 1: Build
# ---------------------------------------------------------------------------
FROM node:20-alpine AS builder

WORKDIR /app

# Build arg — injected from docker-compose at build time
ARG BACKEND_URL=http://backend:8080
ENV BACKEND_URL=$BACKEND_URL
ARG FACE_RECOGNITION_URL=http://face_recognition:8081
ENV FACE_RECOGNITION_URL=$FACE_RECOGNITION_URL

# Copy lockfile if it exists (matches package.json + optionally package-lock.json).
# Use `npm ci` for reproducible installs when the lock is present, fall back to
# `npm install` otherwise — needed because the team repo doesn't ship a lock.
COPY package*.json ./
RUN if [ -f package-lock.json ]; then \
        npm ci --no-audit --prefer-offline; \
    else \
        npm install --no-audit --prefer-offline; \
    fi

COPY . .
RUN npm run build

# ---------------------------------------------------------------------------
# Stage 2: Runtime
# ---------------------------------------------------------------------------
FROM node:20-alpine AS runtime

WORKDIR /app

ENV NODE_ENV=production
ENV PORT=3000

# Pass the URL to runtime as well
ARG BACKEND_URL=http://backend:8080
ENV BACKEND_URL=$BACKEND_URL
ARG FACE_RECOGNITION_URL=http://face_recognition:8081
ENV FACE_RECOGNITION_URL=$FACE_RECOGNITION_URL

COPY --from=builder /app/public ./public
COPY --from=builder /app/.next ./.next
COPY --from=builder /app/node_modules ./node_modules
COPY --from=builder /app/package.json ./package.json

EXPOSE 3000
CMD ["npm", "start"]