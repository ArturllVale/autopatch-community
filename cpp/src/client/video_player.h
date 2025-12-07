#pragma once

#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <evr.h>
#include <string>
#include <functional>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "evr.lib")
#pragma comment(lib, "strmiids.lib")

namespace autopatch
{

    class VideoPlayer : public IMFAsyncCallback
    {
    public:
        VideoPlayer();
        ~VideoPlayer();

        // Inicializa o player com a janela de renderização
        bool Initialize(HWND hwnd);

        // Carrega um vídeo da URL ou caminho local
        bool LoadVideo(const std::wstring &url);

        // Controles de reprodução
        bool Play();
        bool Pause();
        bool Stop();
        bool TogglePlayPause();

        // Estado
        bool IsPlaying() const { return m_isPlaying; }
        bool IsPaused() const { return m_isPaused; }
        bool IsLoaded() const { return m_pSession != nullptr; }

        // Configurações
        void SetLoop(bool loop) { m_loop = loop; }
        void SetMuted(bool muted);
        void SetVolume(float volume); // 0.0 a 1.0

        // Captura o frame atual para usar como imagem estática
        bool CaptureCurrentFrame(BYTE **ppData, UINT32 *pWidth, UINT32 *pHeight);

        // Libera recursos
        void Shutdown();

        // Redimensiona o vídeo quando a janela muda de tamanho
        void ResizeVideo(UINT width, UINT height);

        // IMFAsyncCallback
        STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override;
        STDMETHODIMP_(ULONG)
        AddRef() override;
        STDMETHODIMP_(ULONG)
        Release() override;
        STDMETHODIMP GetParameters(DWORD *, DWORD *) override { return E_NOTIMPL; }
        STDMETHODIMP Invoke(IMFAsyncResult *pAsyncResult) override;

    private:
        bool CreateSession();
        bool CreateMediaSource(const std::wstring &url);
        bool CreateTopology();
        void HandleSessionEvent(IMFMediaEvent *pEvent);

        HWND m_hwnd = nullptr;
        IMFMediaSession *m_pSession = nullptr;
        IMFMediaSource *m_pSource = nullptr;
        IMFVideoDisplayControl *m_pVideoDisplay = nullptr;

        bool m_isPlaying = false;
        bool m_isPaused = false;
        bool m_loop = true;
        bool m_muted = true;

        long m_refCount = 1;

        // Para callbacks
        std::function<void()> m_onEndOfStream;
    };

} // namespace autopatch
