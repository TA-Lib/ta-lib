# ==============================================================================
# TA-Lib Multi-Architecture Dockerfile
# Builds the complete native C library (240+ indicators, headers, static & shared)
# ==============================================================================

# 1. BUILD STAGE
FROM alpine:3.20 AS builder

RUN apk add --no-cache \
    build-base \
    cmake \
    ninja

WORKDIR /src
COPY . .

# Build both static (.a) and shared (.so) libraries, plus universal test runner
RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_STATIC_LIBS=ON \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TESTING=ON \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    && cmake --build build \
    && cmake --install build

# 2. RUNTIME / DISTRIBUTION IMAGE STAGE
FROM alpine:3.20

RUN apk add --no-cache ca-certificates libgcc

# Copy installed C headers, static archives (.a), shared objects (.so), and cmake files
COPY --from=builder /usr/local/include/ /usr/local/include/
COPY --from=builder /usr/local/lib/ /usr/local/lib/
COPY --from=builder /src/bin/ta_regtest /usr/local/bin/ta_regtest

ENV LD_LIBRARY_PATH=/usr/local/lib
ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

WORKDIR /usr/local/bin

# Default command runs the universal regression test verifying all 240 indicators
CMD ["ta_regtest"]
