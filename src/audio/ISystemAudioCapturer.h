#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace fuwa::audio {

// Abstrai a captura de audio do sistema (loopback) por SO. Cada plataforma
// implementa isso de um jeito completamente diferente:
//   - Windows: WASAPI loopback (ver WasapiLoopbackCapturer)
//   - Linux: monitor source via PulseAudio/PipeWire (nao implementado ainda)
//   - macOS: ScreenCaptureKit (nao implementado ainda)
// O MediaController so conhece esta interface, nunca o detalhe de SO.
class ISystemAudioCapturer
{
public:
    // PCM intercalado, 16-bit signed, conforme sampleRate/channels informados
    // em Start(). Chamado de uma thread de captura dedicada - o callback deve
    // ser rapido (so enfileirar os frames para o audio source do webrtc).
    using FrameCallback = std::function<void(const int16_t* samples, size_t sampleCount, int sampleRateHz, int channels)>;

    virtual ~ISystemAudioCapturer() = default;

    virtual bool start(FrameCallback callback) = 0;
    virtual void stop() = 0;
    virtual bool isCapturing() const = 0;
};

// Cria a implementacao apropriada para o SO atual. Retorna nullptr em SOs
// ainda nao suportados (ver README.md - Linux/macOS sao trabalho futuro).
std::unique_ptr<ISystemAudioCapturer> createSystemAudioCapturer();

} // namespace fuwa::audio
