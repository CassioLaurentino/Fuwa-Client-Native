# syntax=docker/dockerfile:1.6
# Build container para o Fuwa-Client-Native. NAO e um runtime - a GUI nunca
# roda dentro deste container (captura de tela/audio e display exigem acesso
# ao host, fragil/inviavel em container). Use "docker build --output" para
# extrair o executavel compilado e empacote-o num instalador nativo fora daqui.
#
# Uso:
#   docker build --target export -o ./dist -f docker/Dockerfile .

FROM ubuntu:22.04 AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build git curl pkg-config \
    ca-certificates python3 \
    && rm -rf /var/lib/apt/lists/*

# Qt6, libmediasoupclient (+ libwebrtc) e sioclient NAO sao buscados aqui -
# sao dependencias pesadas demais para rebuildar a cada imagem (ver README.md).
# Este Dockerfile assume que elas ja foram publicadas num repositorio interno
# de artefatos e sao copiadas/baixadas para /opt/fuwa-native-deps. Ajuste o
# COPY/curl abaixo para o seu artefato real antes de usar em CI.
# COPY --from=fuwa-native-deps:latest / /opt/fuwa-native-deps

WORKDIR /src
COPY . .

RUN cmake -B build -S . -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=/opt/fuwa-native-deps \
    && cmake --build build --config Release

FROM scratch AS export
COPY --from=build /src/build/fuwa-client-native /
COPY --from=build /src/config/fuwa-client.example.json /fuwa-client.json
