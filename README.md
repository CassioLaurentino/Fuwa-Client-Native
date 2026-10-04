# Fuwa-Client-Native

Cliente desktop nativo (C++/Qt) para o [Fuwa-Server](../Fuwa-Server), substituindo
o antigo cliente web ([Fuwa-Client](../Fuwa-Client)). Resolve as duas dores do
cliente anterior:

- **Sem HTTPS/permissões de navegador**: captura de tela e áudio via libwebrtc
  nativo, sem `getDisplayMedia`/contexto seguro.
- **Sem tela preta entre redes diferentes**: usa os `iceServers` (STUN/TURN)
  fornecidos pelo servidor (veja `docker-compose.turn.yml` no Fuwa-Server).

Sem limite de participantes, vídeo selecionável (720p/1080p), áudio do sistema
(loopback) e mute/unmute independente por participante remoto (todos iniciam
mutados).

## Arquitetura

```
src/
├── main.cpp                       # bootstrap Qt + carregamento de config
├── AppConfig.{h,cpp}               # le fuwa-client.json (serverUrl, iceServers)
├── SignalingClient.{h,cpp}         # socket.io-client-cpp, espelha os eventos do servidor
├── MediaController.{h,cpp}         # Device/SendTransport/RecvTransport/Producer/Consumer
│                                    # (libmediasoupclient), orquestra captura + qualidade
├── audio/
│   ├── ISystemAudioCapturer.h      # interface cross-platform de captura de audio loopback
│   └── WasapiLoopbackCapturer.{h,cpp}  # implementacao Windows (WASAPI loopback)
└── ui/
    ├── MainWindow.{h,cpp}          # tela de conexao + grid + seletor de qualidade
    └── VideoTile.{h,cpp}           # 1 tile: render de video + botao mute/unmute
```

### Protocolo de sinalização

Reaproveita 1:1 os eventos já existentes no Fuwa-Server — nenhuma mudança de
protocolo foi necessária, só reimplementar o lado cliente em C++:
`join`, `createTransport`, `connectTransport`, `produce`, `consume`,
`resumeConsumer`, `getProducers`, `newProducer`, `peerClosed`, `consumerClosed`.

## Pré-requisitos de build (não vendorizados neste repositório)

Este projeto depende de bibliotecas nativas pesadas que precisam ser
compiladas/instaladas separadamente antes do `cmake --build`:

1. **Qt 6** (Widgets) — via instalador oficial ou `aqtinstall`.
2. **[libmediasoupclient](https://github.com/versatica/libmediasoupclient)**,
   que por sua vez exige uma build de **libwebrtc** (processo oficial via
   `depot_tools`, documentado no próprio repositório do libmediasoupclient).
   Isso é tipicamente um processo de horas e dezenas de GB de disco — não é
   algo que deva ser refeito a cada build de aplicação, então trate o
   artefato resultante como uma dependência versionada/cacheada (ex.: publique
   os binários compilados num repositório interno de artefatos e aponte
   `CMAKE_PREFIX_PATH` para eles).
3. **[socket.io-client-cpp](https://github.com/socketio/socket.io-client-cpp)**
   (traz `websocketpp`/`asio`/`rapidjson` como dependências transitivas).
4. **nlohmann/json** (usado pelo libmediasoupclient e por este projeto).

Depois de ter tudo instalado/buildado, aponte o CMake para os prefixos:

```powershell
cmake -B build -S . `
  -DCMAKE_PREFIX_PATH="C:/libs/qt6;C:/libs/libmediasoupclient;C:/libs/sioclient" `
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Docker: build container (NÃO é o runtime)

`docker/Dockerfile` só compila e exporta o executável/instalador — ele **não**
roda a GUI dentro do container. Captura de tela/áudio e exibição de janelas
exigem acesso ao display/host, o que é frágil (Linux) ou inviável (Windows via
Docker Desktop) dentro de um container. Use o container como ambiente de build
reprodutível/CI:

```powershell
docker build --target export -o ./dist -f docker/Dockerfile .
```

O artefato final em `./dist` deve ser empacotado com um instalador nativo
(NSIS/Inno Setup no Windows, `.deb`/AppImage no Linux) para distribuição ao
usuário final — isso ainda não está automatizado neste scaffold.

## Configuração (sem rebuild por ambiente)

O executável lê `fuwa-client.json` (mesma pasta do binário, ou caminho via
variável de ambiente `FUWA_CLIENT_CONFIG`) — veja
[`config/fuwa-client.example.json`](config/fuwa-client.example.json). Contém
apenas `serverUrl`; a lista de `iceServers` (STUN/TURN) é obtida em tempo de
execução a partir do servidor (evento `join`), então não precisa ser
duplicada na config local.

## Estado deste scaffold

Implementado (estrutura e lógica de orquestração):
- Carregamento de config, cliente de sinalização, orquestração de
  transports/producers/consumers, interface de captura de áudio, UI Qt com
  grid dinâmico, seletor de qualidade e mute por tile (iniciando mutado).

Pendente de um ambiente de build nativo real para finalizar/validar:
- Build real de libwebrtc/libmediasoupclient e wiring fino dos sinks de vídeo
  (frame → textura Qt) — os pontos de integração estão marcados com `// TODO`.
- Implementações de loopback de áudio para Linux (PulseAudio/PipeWire) e
  macOS (ScreenCaptureKit) — só o Windows (WASAPI) está implementado.
- Geração de instaladores nativos (NSIS/Inno Setup/AppImage).
