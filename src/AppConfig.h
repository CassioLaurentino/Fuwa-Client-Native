#pragma once

#include <string>
#include <vector>

namespace fuwa {

// Representa um iceServer (STUN/TURN) recebido do servidor no evento "join"/
// "createTransport" - nunca lido da config local, so a definicao do schema.
struct IceServer
{
    std::vector<std::string> urls;
    std::string username;
    std::string credential;
};

enum class VideoQuality
{
    P720,
    P1080,
};

std::string toString(VideoQuality quality);
VideoQuality videoQualityFromString(const std::string& value);

// Config local do app (fuwa-client.json), lida uma unica vez na inicializacao.
// Mantem o binario identico entre ambientes - so muda o que aponta para o servidor.
struct AppConfig
{
    std::string serverUrl = "http://localhost:3000";
    VideoQuality defaultQuality = VideoQuality::P720;

    // Procura o arquivo de config na seguinte ordem:
    //   1. variavel de ambiente FUWA_CLIENT_CONFIG (caminho completo)
    //   2. fuwa-client.json ao lado do executavel
    // Lanca std::runtime_error se nenhum arquivo valido for encontrado.
    static AppConfig load();
};

} // namespace fuwa
