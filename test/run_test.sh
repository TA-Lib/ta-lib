#!/bin/sh
set -e

echo "1. Pulling latest image from Docker Hub..."
docker pull rajnish93/ta-lib:latest

echo "2. Building test container..."
docker build -t talib-verifier -f Dockerfile .

echo "3. Running verification..."
docker run --rm talib-verifier

