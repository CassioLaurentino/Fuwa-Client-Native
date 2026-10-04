#pragma once

#include <functional>
#include <string>

#include <nlohmann/json.hpp>
#include <sio_client.h>

namespace fuwa {

// Espelha o protocolo de sinalizacao ja existente no Fuwa-Server (Socket.IO).
// Nenhuma mudanca de protocolo foi necessaria - so reimplementar o lado
// cliente em C++. Eventos: join, createTransport, connectTransport, produce,
// consume, resumeConsumer, getProducers, newProducer, peerClosed, consumerClosed.
class SignalingClient
{
public:
    using ResponseCallback = std::function<void(nlohmann::json response)>;
    using NewProducerHandler = std::function<void(const std::string& producerId, const std::string& peerId)>;
    using PeerClosedHandler = std::function<void(const std::string& peerId)>;
    using ConsumerClosedHandler = std::function<void(const std::string& consumerId)>;

    explicit SignalingClient(std::string serverUrl);
    ~SignalingClient();

    void connect();
    void disconnect();

    // Equivalente ao socketRequest() do cliente web antigo: emite um evento e
    // resolve o callback (ack) do Socket.IO como resposta, incluindo o padrao
    // de erro { error: "..." } usado em todos os handlers do servidor.
    void request(const std::string& event, const nlohmann::json& data, ResponseCallback callback);

    void onNewProducer(NewProducerHandler handler) { newProducerHandler_ = std::move(handler); }
    void onPeerClosed(PeerClosedHandler handler) { peerClosedHandler_ = std::move(handler); }
    void onConsumerClosed(ConsumerClosedHandler handler) { consumerClosedHandler_ = std::move(handler); }

private:
    void bindEventHandlers();

    std::string serverUrl_;
    sio::client client_;

    NewProducerHandler newProducerHandler_;
    PeerClosedHandler peerClosedHandler_;
    ConsumerClosedHandler consumerClosedHandler_;
};

} // namespace fuwa
