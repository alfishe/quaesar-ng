FROM ubuntu:22.04

LABEL description="Build quaesar-ng on Linux (headless compile-only)"

# Avoid interactive tzdata prompt
ENV DEBIAN_FRONTEND=noninteractive

# Install build toolchain and all required dev packages
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    clang \
    libsdl2-dev \
    libgtk-3-dev \
    zlib1g-dev \
    pkg-config \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copy the entire source tree (build context is repo root, see .dockerignore)
COPY . /build/src

# Configure (Release) with Ninja generator
RUN cmake -S /build/src -B /build/release \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_COMPILER=clang

# Build
RUN cmake --build /build/release --target quaesar -j$(nproc)

# The binary lands at /build/src/quaesar per RUNTIME_OUTPUT_DIRECTORY in CMakeLists.txt
RUN ls -la /build/src/quaesar && echo "Build successful"
