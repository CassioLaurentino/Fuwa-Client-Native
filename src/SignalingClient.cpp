#include "SignalingClient.h"

#include <iostream>

// Conversao minima entre nlohmann::json e sio::message. Projetos reais devem
// isolar isso num helper compartilhado (socket.io-client-cpp usa sio::message,
// nao json de verdade) - aqui mantido simples e explicito para os poucos
// formatos que o protocolo do Fuwa-Server realmente usa (objetos planos).
namespace {

sio::message::ptr jsonToSioMessage(const nlohmann::json& json)
{
    auto message = sio::object_message::create();
    for (auto it = json.begin(); it != json.end(); ++it)
    {
        if (it->is_string())
            message->get_map()[it.key()] = sio::string_message::create(it->get<std::string>());
        else if (it->is_number_integer())
            message->get_map()[it.key()] = sio::int_message::create(it->get<int64_t>());
        else if (it->is_boolean())
            message->get_map()[it.key()] = sio::bool_message::create(it->get<bool>());
        else
            message->get_map()[it.key()] = sio::string_message::create(it->dump());
    }
    return message;
}

nlohmann::json sioMessageToJson(const sio::message::ptr& message)
{
    // TODO: converter recursivamente para arrays/objetos aninhados reais.
    // O protocolo atual do Fuwa-Server usa, no pior caso, objetos com 1 nivel
    // de aninhamento (ex.: iceParameters dentro de createTransport) - validar
    // caso a caso ao integrar com o servidor real.
    nlohmann::json json = nlohmann::json::object();
    if (message->get_flag() == sio::message::flag_object)
    {
        for (auto& [key, value] : message->get_map())
        {
            if (value->get_flag() == sio::message::flag_string)
                json[key] = value->get_string();
            else if (value->get_flag() == sio::message::flag_integer)
                json[key] = value->get_int();
            else if (value->get_flag() == sio::message::flag_boolean)
                json[key] = value->get_bool();
        }
    }
    return json;
}

} // namespace

namespace fuwa {

SignalingClient::SignalingClient(std::string serverUrl) : serverUrl_(std::move(serverUrl))
{
}

SignalingClient::~SignalingClient()
{
    disconnect();
}

void SignalingClient::connect()
{
    bindEventHandlers();
    client_.connect(serverUrl_);
}

void SignalingClient::disconnect()
{
    client_.sync_close();
}

void SignalingClient::request(const std::string& event, const nlohmann::json& data, ResponseCallback callback)
{
    client_.socket()->emit(event, jsonToSioMessage(data), [callback = std::move(callback)](const sio::message::list& ack) {
        nlohmann::json response = ack.size() > 0 ? sioMessageToJson(ack[0]) : nlohmann::json::object();
        callback(std::move(response));
    });
}

void SignalingClient::bindEventHandlers()
{
    client_.socket()->on("newProducer", sio::socket::event_listener([this](sio::event& event) {
        auto json = sioMessageToJson(event.get_message());
        if (newProducerHandler_)
            newProducerHandler_(json.value("producerId", ""), json.value("peerId", ""));
    }));

    client_.socket()->on("peerClosed", sio::socket::event_listener([this](sio::event& event) {
        auto json = sioMessageToJson(event.get_message());
        if (peerClosedHandler_)
            peerClosedHandler_(json.value("peerId", ""));
    }));

    client_.socket()->on("consumerClosed", sio::socket::event_listener([this](sio::event& event) {
        auto json = sioMessageToJson(event.get_message());
        if (consumerClosedHandler_)
            consumerClosedHandler_(json.value("consumerId", ""));
    }));
}

} // namespace fuwa
