# ==============================================================================
# Verification Container
# Pulls rajnish93/ta-lib:latest and verifies all 240 indicators in Go
# ==============================================================================

# 1. PULL PUBLISHED TA-LIB IMAGE FROM DOCKER HUB
FROM rajnish93/ta-lib:latest AS talib-image

# 2. GO RUNTIME WITH GCC / CGO
FROM golang:1.23-alpine

RUN apk add --no-cache build-base

# Copy compiled TA-Lib headers and libraries from the pulled Docker Hub image
COPY --from=talib-image /usr/local/include/ /usr/local/include/
COPY --from=talib-image /usr/local/lib/ /usr/local/lib/

ENV LD_LIBRARY_PATH=/usr/local/lib
ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

WORKDIR /app

# Copy Go test code
COPY go.mod main.go ./

# Compile Go program linking against the pulled TA-Lib image
RUN CGO_ENABLED=1 go build -o /app/verify-talib .

CMD ["/app/verify-talib"]

