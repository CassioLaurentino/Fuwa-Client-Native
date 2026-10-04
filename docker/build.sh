#!/usr/bin/env bash
set -euo pipefail

# Builda o Fuwa-Client-Native via Docker (container de build, nao runtime -
# ver docker/Dockerfile) e extrai o executavel para ./dist.
cd "$(dirname "$0")/.."

docker build --target export -o ./dist -f docker/Dockerfile .

echo "Binario exportado em ./dist"
