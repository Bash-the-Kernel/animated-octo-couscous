# Multi-stage build for HFT Exchange

# Builder stage
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    ninja-build \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copy source
COPY . .

# Build
RUN mkdir -p build && cd build && \
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . -j$(nproc)

# Runtime stage
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN useradd -m -u 1000 hft && \
    mkdir -p /app/data && \
    chown -R hft:hft /app

WORKDIR /app

# Copy binary from builder
COPY --from=builder /build/build/hft_exchange /app/
COPY --from=builder /build/build/libhft_core.a /app/

# Copy config
COPY config.json /app/

USER hft

EXPOSE 8080 9002

VOLUME ["/app/data"]

ENTRYPOINT ["/app/hft_exchange"]
CMD ["config.json"]
