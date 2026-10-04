#pragma once

#include "ISystemAudioCapturer.h"

#include <atomic>
#include <thread>

// Forward decls para nao vazar <audioclient.h>/<mmdeviceapi.h> no header.
struct IAudioClient;
struct IAudioCaptureClient;
struct IMMDevice;

namespace fuwa::audio {

// Captura o audio que esta sendo reproduzido pelo sistema (loopback),
// equivalente ao que o getDisplayMedia faz no Chrome ao marcar "compartilhar
// audio do sistema" - mas sem depender de navegador. Usa o dispositivo de
// renderizacao padrao em modo loopback (WASAPI), disponivel desde Windows Vista.
class WasapiLoopbackCapturer final : public ISystemAudioCapturer
{
public:
    WasapiLoopbackCapturer();
    ~WasapiLoopbackCapturer() override;

    bool start(FrameCallback callback) override;
    void stop() override;
    bool isCapturing() const override { return capturing_.load(); }

private:
    void captureLoop();

    FrameCallback callback_;
    std::atomic<bool> capturing_{false};
    std::thread captureThread_;

    IMMDevice* device_ = nullptr;
    IAudioClient* audioClient_ = nullptr;
    IAudioCaptureClient* captureClient_ = nullptr;

    int sampleRateHz_ = 48000;
    int channels_ = 2;
};

} // namespace fuwa::audio
