# ==========================================
# 1. BUILD STAGE (Pure Go, Zero-CGO)
# ==========================================
FROM --platform=$BUILDPLATFORM golang:1.23-alpine AS builder

ARG TARGETOS
ARG TARGETARCH

WORKDIR /app

# Install CA certificates for secure HTTPS/WSS exchange connectivity
RUN apk --no-cache add ca-certificates

# Cache dependencies
COPY go/go.mod ./
RUN go mod download

# Copy Go engine source
COPY go/ .

# Compile static pure Go binary with CGO disabled
RUN CGO_ENABLED=0 GOOS=$TARGETOS GOARCH=$TARGETARCH \
    go build -ldflags="-s -w" -o /app/ta-engine .

# ==========================================
# 2. RUNTIME STAGE (Zero OS dependencies, ~6MB)
# ==========================================
FROM scratch

# Copy root CA certificates
COPY --from=builder /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/

# Copy statically linked pure Go binary
COPY --from=builder /app/ta-engine /ta-engine

EXPOSE 8080

ENTRYPOINT ["/ta-engine"]

