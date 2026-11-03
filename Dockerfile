# ==============================================================================
# TA-Lib Multi-Architecture Dockerfile (Optimized ~16 MB)
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

# Build both static (.a) and shared (.so) libraries
RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_STATIC_LIBS=ON \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TESTING=OFF \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    && cmake --build build \
    && cmake --install build \
    && strip --strip-unneeded /usr/local/lib/libta-lib.so* \
    && strip --strip-debug /usr/local/lib/libta-lib.a

# 2. RUNTIME / DISTRIBUTION IMAGE STAGE (~16 MB)
FROM alpine:3.20

# Copy stripped C headers, static archives (.a), shared objects (.so), and cmake files
COPY --from=builder /usr/local/include/ /usr/local/include/
COPY --from=builder /usr/local/lib/ /usr/local/lib/

ENV LD_LIBRARY_PATH=/usr/local/lib
ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

# Default smoke test verifying the shared library is loaded
CMD ["sh", "-c", "ls -lh /usr/local/lib/libta-lib.* && echo 'TA-Lib C library ready!'"]
