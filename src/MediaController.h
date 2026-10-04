#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>

#include <mediasoupclient.hpp>

#include "AppConfig.h"
#include "SignalingClient.h"
#include "audio/ISystemAudioCapturer.h"

namespace fuwa {

// Estado de uma transmissao remota exibida na UI - 1 por peer remoto que
// esta publicando video (o audio do mesmo peer e atrelado ao mesmo tile).
struct RemoteTrackHandle
{
    std::string peerId;
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> videoTrack;
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> audioTrack;
    bool muted = true; // todas as transmissoes remotas iniciam mutadas (requisito de produto)
};

// Orquestra Device/SendTransport/RecvTransport/Producer/Consumer do
// libmediasoupclient, a captura de tela+audio do sistema, e a troca de
// qualidade de video. E o unico lugar que fala tanto com o SignalingClient
// quanto com o mediasoup-client - a UI (MainWindow) so conhece esta classe.
class MediaController
{
public:
    using RemoteTrackAdded = std::function<void(const RemoteTrackHandle&)>;
    using RemoteTrackRemoved = std::function<void(const std::string& peerId)>;

    explicit MediaController(AppConfig config);
    ~MediaController();

    // Conecta ao servidor, carrega o Device com as RTP capabilities da sala,
    // cria os transports de envio/recebimento e comeca a publicar tela+audio.
    void start();
    void stop();

    // Troca a resolucao/bitrate de captura em tempo real (720p/1080p).
    // Reinicia o capturer de video e recria o producer correspondente -
    // ver nota no .cpp sobre por que no-renegotiation (SetParameters) nao
    // cobre mudanca de resolucao de captura, so de bitrate.
    void setVideoQuality(VideoQuality quality);

    // Mute/unmute e sempre local (reproducao) - nunca afeta o que o peer
    // remoto esta enviando aos demais participantes.
    void setRemoteMuted(const std::string& peerId, bool muted);

    void onRemoteTrackAdded(RemoteTrackAdded handler) { remoteTrackAdded_ = std::move(handler); }
    void onRemoteTrackRemoved(RemoteTrackRemoved handler) { remoteTrackRemoved_ = std::move(handler); }

private:
    void createSendTransport(const nlohmann::json& rtpCapabilities);
    void createRecvTransport();
    void startPublishing();
    void consumeProducer(const std::string& producerId, const std::string& peerId);
    void removePeerTiles(const std::string& peerId);

    // Mapeia qualidade -> constraints de captura + teto de bitrate do encoding.
    struct QualityProfile
    {
        int width;
        int height;
        int maxBitrateBps;
    };
    static QualityProfile profileFor(VideoQuality quality);

    AppConfig config_;
    SignalingClient signaling_;

    std::unique_ptr<mediasoupclient::Device> device_;
    std::unique_ptr<mediasoupclient::SendTransport> sendTransport_;
    std::unique_ptr<mediasoupclient::RecvTransport> recvTransport_;

    std::unique_ptr<mediasoupclient::Producer> videoProducer_;
    std::unique_ptr<mediasoupclient::Producer> audioProducer_;
    std::map<std::string, std::unique_ptr<mediasoupclient::Consumer>> consumersByPeerId_;

    std::unique_ptr<audio::ISystemAudioCapturer> systemAudio_;

    VideoQuality currentQuality_;
    std::string peerId_;

    RemoteTrackAdded remoteTrackAdded_;
    RemoteTrackRemoved remoteTrackRemoved_;
};

} // namespace fuwa
