#include "AppConfig.h"

#include <cstdlib>
#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace fuwa {

std::string toString(VideoQuality quality)
{
    switch (quality)
    {
        case VideoQuality::P1080:
            return "1080p";
        case VideoQuality::P720:
        default:
            return "720p";
    }
}

VideoQuality videoQualityFromString(const std::string& value)
{
    if (value == "1080p")
        return VideoQuality::P1080;
    return VideoQuality::P720;
}

namespace {

// Caminho do executavel + fuwa-client.json, independente do cwd atual.
std::string defaultConfigPath()
{
    QFileInfo exeInfo(QCoreApplication::applicationFilePath());
    return exeInfo.dir().filePath("fuwa-client.json").toStdString();
}

} // namespace

AppConfig AppConfig::load()
{
    std::string path = defaultConfigPath();

    if (const char* envPath = std::getenv("FUWA_CLIENT_CONFIG"))
        path = envPath;

    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error(
            "Arquivo de configuracao nao encontrado: " + path +
            " (defina FUWA_CLIENT_CONFIG ou coloque fuwa-client.json ao lado do executavel)");
    }

    nlohmann::json json;
    file >> json;

    AppConfig config;
    config.serverUrl = json.value("serverUrl", config.serverUrl);
    config.defaultQuality = videoQualityFromString(json.value("defaultQuality", std::string("720p")));

    return config;
}

} // namespace fuwa
