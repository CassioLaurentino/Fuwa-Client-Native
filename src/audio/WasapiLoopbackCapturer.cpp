#include "WasapiLoopbackCapturer.h"

#include <audioclient.h>
#include <mmdeviceapi.h>

#include <avrt.h>
#include <combaseapi.h>

#include <stdexcept>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")

// Implementacao WASAPI loopback - captura o audio que o Windows esta
// mandando para o dispositivo de saida padrao (equivalente ao "audio do
// sistema" do getDisplayMedia, so que sem navegador). Baseado no padrao
// oficial da Microsoft para "loopback recording":
// https://learn.microsoft.com/windows/win32/coreaudio/loopback-recording

namespace fuwa::audio {

namespace {

template <typename Fn>
void mustSucceed(HRESULT hr, Fn onFailureCleanup)
{
    if (FAILED(hr))
    {
        onFailureCleanup();
        throw std::runtime_error("Falha WASAPI, HRESULT=" + std::to_string(hr));
    }
}

} // namespace

std::unique_ptr<ISystemAudioCapturer> createSystemAudioCapturer()
{
    return std::make_unique<WasapiLoopbackCapturer>();
}

WasapiLoopbackCapturer::WasapiLoopbackCapturer() = default;

WasapiLoopbackCapturer::~WasapiLoopbackCapturer()
{
    stop();
}

bool WasapiLoopbackCapturer::start(FrameCallback callback)
{
    if (capturing_.load())
        return true;

    callback_ = std::move(callback);

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        return false;

    IMMDeviceEnumerator* enumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                           __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr))
        return false;

    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device_);
    enumerator->Release();
    if (FAILED(hr))
        return false;

    hr = device_->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&audioClient_));
    if (FAILED(hr))
        return false;

    WAVEFORMATEX* mixFormat = nullptr;
    hr = audioClient_->GetMixFormat(&mixFormat);
    if (FAILED(hr))
        return false;

    sampleRateHz_ = static_cast<int>(mixFormat->nSamplesPerSec);
    channels_ = mixFormat->nChannels;

    // AUDCLNT_STREAMFLAGS_LOOPBACK: captura o que esta sendo reproduzido,
    // nao um microfone. Buffer de 200ms e suficiente para baixa latencia
    // sem acumular jitter excessivo.
    hr = audioClient_->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
                                   2'000'000, 0, mixFormat, nullptr);
    CoTaskMemFree(mixFormat);
    if (FAILED(hr))
        return false;

    hr = audioClient_->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&captureClient_));
    if (FAILED(hr))
        return false;

    hr = audioClient_->Start();
    if (FAILED(hr))
        return false;

    capturing_.store(true);
    captureThread_ = std::thread(&WasapiLoopbackCapturer::captureLoop, this);
    return true;
}

void WasapiLoopbackCapturer::stop()
{
    if (!capturing_.exchange(false))
        return;

    if (captureThread_.joinable())
        captureThread_.join();

    if (audioClient_)
        audioClient_->Stop();

    if (captureClient_)
    {
        captureClient_->Release();
        captureClient_ = nullptr;
    }
    if (audioClient_)
    {
        audioClient_->Release();
        audioClient_ = nullptr;
    }
    if (device_)
    {
        device_->Release();
        device_ = nullptr;
    }
}

void WasapiLoopbackCapturer::captureLoop()
{
    DWORD taskIndex = 0;
    HANDLE avrtHandle = AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex);

    while (capturing_.load())
    {
        UINT32 packetLength = 0;
        if (FAILED(captureClient_->GetNextPacketSize(&packetLength)))
            break;

        while (packetLength != 0)
        {
            BYTE* data = nullptr;
            UINT32 framesAvailable = 0;
            DWORD flags = 0;

            if (FAILED(captureClient_->GetBuffer(&data, &framesAvailable, &flags, nullptr, nullptr)))
                break;

            if (callback_ && !(flags & AUDCLNT_BUFFERFLAGS_SILENT))
            {
                callback_(reinterpret_cast<const int16_t*>(data),
                           static_cast<size_t>(framesAvailable) * channels_, sampleRateHz_, channels_);
            }

            captureClient_->ReleaseBuffer(framesAvailable);

            if (FAILED(captureClient_->GetNextPacketSize(&packetLength)))
            {
                packetLength = 0;
                break;
            }
        }

        Sleep(10);
    }

    if (avrtHandle)
        AvRevertMmThreadCharacteristics(avrtHandle);
}

} // namespace fuwa::audio
