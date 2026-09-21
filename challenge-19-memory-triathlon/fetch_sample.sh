#!/usr/bin/env bash
# Download the local-dev sample trace (not committed — it's a ~11 MB binary).
# CMake fetches it into the build dir automatically on first configure; this is
# the manual fallback (offline/CI/firewalled).
#
# Usage: ./fetch_sample.sh [output_path]   (default: build/sample.trace)
set -euo pipefail
URL="https://github.com/Vitorian/hft-challenges/releases/download/c19-sample-v2/sample.trace"
SHA256="01cb06389c66fc411af22605d2f04c179fa55bbefc9a8fa26e55674ee98a7bf3"
DIR="$(cd "$(dirname "$0")" && pwd)"
OUT="${1:-$DIR/build/sample.trace}"

mkdir -p "$(dirname "$OUT")"
echo "Fetching sample.trace -> $OUT"
curl -fSL "$URL" -o "$OUT"

if command -v sha256sum >/dev/null 2>&1; then
    echo "${SHA256}  ${OUT}" | sha256sum -c - || { echo "checksum mismatch"; exit 1; }
fi
echo "OK"
