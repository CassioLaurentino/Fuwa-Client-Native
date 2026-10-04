#include "MediaController.h"

#include <future>
#include <iostream>

// NOTA DE VERSAO: as assinaturas exatas de mediasoupclient::Device /
// SendTransport::Listener / RecvTransport::Listener variam entre versoes do
// libmediasoupclient. O que esta aqui reflete a API publica documentada no
// repositorio oficial (versatica/libmediasoupclient) - confira os headers da
// versao realmente instalada (`mediasoupclient.hpp`) antes de compilar, e
// ajuste assinaturas se necessario. A logica de orquestracao (ordem das
// chamadas, o que fica local vs. remoto) e a parte que nao muda entre versoes.

namespace fuwa {

namespace {

// Listener do SendTransport: toda vez que o mediasoup-client precisa
// conectar o transporte ou publicar um producer, ele chama aqui - nos so
// repassamos para o SignalingClient e devolvemos a resposta via future,
// exatamente como o "sendTransport.on('connect'/'produce')" fazia no
// cliente web antigo (ver Fuwa-Client/src/main.js).
class SendTransportListener : public mediasoupclient::SendTransport::Listener
{
public:
    explicit SendTransportListener(SignalingClient& signaling) : signaling_(signaling) {}

    std::future<void> OnConnect(mediasoupclient::SendTransport* transport, const nlohmann::json& dtlsParameters) override
    {
        auto promise = std::make_shared<std::promise<void>>();
        nlohmann::json payload{{"transportId", transport->GetId()}, {"dtlsParameters", dtlsParameters}};
        signaling_.request("connectTransport", payload, [promise](nlohmann::json response) {
            if (response.contains("error"))
                promise->set_exception(std::make_exception_ptr(std::runtime_error(response["error"].get<std::string>())));
            else
                promise->set_value();
        });
        return promise->get_future();
    }

    void OnConnectionStateChange(mediasoupclient::SendTransport*, const std::string& state) override
    {
        std::cout << "[sendTransport] estado: " << state << std::endl;
    }

    std::future<std::string> OnProduce(mediasoupclient::SendTransport* transport, const std::string& kind,
                                        nlohmann::json rtpParameters, const nlohmann::json&) override
    {
        auto promise = std::make_shared<std::promise<std::string>>();
        nlohmann::json payload{{"transportId", transport->GetId()}, {"kind", kind}, {"rtpParameters", rtpParameters}};
        signaling_.request("produce", payload, [promise](nlohmann::json response) {
            if (response.contains("error"))
                promise->set_exception(std::make_exception_ptr(std::runtime_error(response["error"].get<std::string>())));
            else
                promise->set_value(response["id"].get<std::string>());
        });
        return promise->get_future();
    }

private:
    SignalingClient& signaling_;
};

class RecvTransportListener : public mediasoupclient::RecvTransport::Listener
{
public:
    explicit RecvTransportListener(SignalingClient& signaling) : signaling_(signaling) {}

    std::future<void> OnConnect(mediasoupclient::RecvTransport* transport, const nlohmann::json& dtlsParameters) override
    {
        auto promise = std::make_shared<std::promise<void>>();
        nlohmann::json payload{{"transportId", transport->GetId()}, {"dtlsParameters", dtlsParameters}};
        signaling_.request("connectTransport", payload, [promise](nlohmann::json response) {
            if (response.contains("error"))
                promise->set_exception(std::make_exception_ptr(std::runtime_error(response["error"].get<std::string>())));
            else
                promise->set_value();
        });
        return promise->get_future();
    }

    void OnConnectionStateChange(mediasoupclient::RecvTransport*, const std::string& state) override
    {
        std::cout << "[recvTransport] estado: " << state << std::endl;
    }

private:
    SignalingClient& signaling_;
};

} // namespace

MediaController::MediaController(AppConfig config)
    : config_(std::move(config)), signaling_(config_.serverUrl), currentQuality_(config_.defaultQuality)
{
}

MediaController::~MediaController()
{
    stop();
}

MediaController::QualityProfile MediaController::profileFor(VideoQuality quality)
{
    switch (quality)
    {
        case VideoQuality::P1080:
            return {1920, 1080, 4'000'000};
        case VideoQuality::P720:
        default:
            return {1280, 720, 1'500'000};
    }
}

void MediaController::start()
{
    signaling_.onNewProducer([this](const std::string& producerId, const std::string& peerId) {
        consumeProducer(producerId, peerId);
    });
    signaling_.onPeerClosed([this](const std::string& peerId) { removePeerTiles(peerId); });

    signaling_.connect();

    signaling_.request("join", {}, [this](nlohmann::json response) {
        if (response.contains("error"))
        {
            std::cerr << "Falha ao entrar na sala: " << response["error"] << std::endl;
            return;
        }

        peerId_ = response["peerId"].get<std::string>();

        device_ = std::make_unique<mediasoupclient::Device>();
        device_->Load(response["rtpCapabilities"]);

        createSendTransport(response["rtpCapabilities"]);
        createRecvTransport();
        startPublishing();

        signaling_.request("getProducers", {}, [this](nlohmann::json producers) {
            for (auto& item : producers)
                consumeProducer(item["producerId"].get<std::string>(), item["peerId"].get<std::string>());
        });
    });
}

void MediaController::stop()
{
    if (systemAudio_)
        systemAudio_->stop();

    consumersByPeerId_.clear();
    videoProducer_.reset();
    audioProducer_.reset();
    sendTransport_.reset();
    recvTransport_.reset();
    device_.reset();

    signaling_.disconnect();
}

void MediaController::createSendTransport(const nlohmann::json&)
{
    signaling_.request("createTransport", {}, [this](nlohmann::json transportInfo) {
        if (transportInfo.contains("error"))
        {
            std::cerr << "Falha ao criar transporte de envio: " << transportInfo["error"] << std::endl;
            return;
        }

        static SendTransportListener listener(signaling_);
        sendTransport_.reset(device_->CreateSendTransport(
            &listener, transportInfo["id"].get<std::string>(), transportInfo["iceParameters"],
            transportInfo["iceCandidates"], transportInfo["dtlsParameters"]));

        // transportInfo["iceServers"] (STUN/TURN) deve ser repassado nas
        // PeerConnection::Options ao criar o transporte - ver nota de versao
        // no topo do arquivo para o ponto exato de injecao na API instalada.
    });
}

void MediaController::createRecvTransport()
{
    signaling_.request("createTransport", {}, [this](nlohmann::json transportInfo) {
        if (transportInfo.contains("error"))
        {
            std::cerr << "Falha ao criar transporte de recebimento: " << transportInfo["error"] << std::endl;
            return;
        }

        static RecvTransportListener listener(signaling_);
        recvTransport_.reset(device_->CreateRecvTransport(
            &listener, transportInfo["id"].get<std::string>(), transportInfo["iceParameters"],
            transportInfo["iceCandidates"], transportInfo["dtlsParameters"]));
    });
}

void MediaController::startPublishing()
{
    // TODO: criar a track de video a partir do capturador de desktop do
    // libwebrtc (equivalente nativo ao getDisplayMedia, sem navegador) usando
    // o profile de qualidade atual (profileFor(currentQuality_)), e publicar:
    //   videoProducer_.reset(sendTransport_->Produce(listener, videoTrack, &encodings, nullptr, appData));
    // onde encodings[0].max_bitrate_bps = profileFor(currentQuality_).maxBitrateBps.

    // TODO: iniciar o systemAudio_ (WasapiLoopbackCapturer no Windows),
    // alimentar um webrtc::AudioSourceInterface customizado com os frames
    // PCM recebidos, e publicar o producer de audio (codec Opus, ja
    // declarado no router do servidor - ver Fuwa-Server/src/config.js).
    systemAudio_ = audio::createSystemAudioCapturer();
    if (systemAudio_)
    {
        systemAudio_->start([](const int16_t*, size_t, int, int) {
            // Encaminhar para o audio source do producer de audio (TODO acima).
        });
    }
}

void MediaController::setVideoQuality(VideoQuality quality)
{
    if (quality == currentQuality_)
        return;

    currentQuality_ = quality;

    // Mudar resolucao de captura exige recriar o capturer (nao da so para
    // ajustar via SetParameters, que so cobre bitrate/escala de um encoding
    // ja existente). Caminho: parar o producer de video atual, recapturar
    // com o novo profileFor(quality), e produzir de novo.
    videoProducer_.reset();
    startPublishing();
}

void MediaController::setRemoteMuted(const std::string& peerId, bool muted)
{
    // Mute e local: so habilita/desabilita a reproducao do audio recebido
    // deste peer (track->set_enabled(false) no audio track remoto), nunca
    // manda nada de volta ao servidor nem afeta o producer remoto.
    (void)peerId;
    (void)muted;
    // TODO: localizar o audio track do RemoteTrackHandle correspondente e
    // chamar track->set_enabled(!muted).
}

void MediaController::consumeProducer(const std::string& producerId, const std::string& peerId)
{
    nlohmann::json payload{
        {"transportId", recvTransport_->GetId()},
        {"producerId", producerId},
        {"rtpCapabilities", device_->GetRtpCapabilities()},
    };

    signaling_.request("consume", payload, [this, peerId](nlohmann::json response) {
        if (response.contains("error"))
        {
            std::cerr << "Falha ao consumir producer: " << response["error"] << std::endl;
            return;
        }

        static mediasoupclient::Consumer::Listener noopListener; // sem callbacks relevantes aqui

        auto* consumer = recvTransport_->Consume(&noopListener, response["id"].get<std::string>(),
                                                  response["producerId"].get<std::string>(),
                                                  response["kind"].get<std::string>(), &response["rtpParameters"]);
        consumersByPeerId_[peerId].reset(consumer);

        signaling_.request("resumeConsumer", {{"consumerId", consumer->GetId()}}, [](nlohmann::json) {});

        RemoteTrackHandle handle;
        handle.peerId = peerId;
        handle.muted = true; // requisito: toda transmissao remota inicia mutada
        // TODO: popular handle.videoTrack/audioTrack a partir de consumer->GetTrack(),
        // agrupando video+audio do mesmo peerId no mesmo tile.
        if (remoteTrackAdded_)
            remoteTrackAdded_(handle);
    });
}

void MediaController::removePeerTiles(const std::string& peerId)
{
    consumersByPeerId_.erase(peerId);
    if (remoteTrackRemoved_)
        remoteTrackRemoved_(peerId);
}

} // namespace fuwa
