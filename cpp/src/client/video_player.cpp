#include "video_player.h"
#include <Shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

namespace autopatch
{

    // GUID para obter o Video Display Control
    // {C56E9858-68D2-44b5-B61F-9D8D7F63A8D9}
    static const GUID CLSID_EnhancedVideoRenderer =
        {0xfa10746c, 0x9b63, 0x4b6c, {0xbc, 0x49, 0xfc, 0x30, 0x0e, 0xa5, 0xf2, 0x56}};

    VideoPlayer::VideoPlayer()
    {
        // Inicializa Media Foundation
        MFStartup(MF_VERSION);
    }

    VideoPlayer::~VideoPlayer()
    {
        Shutdown();
        MFShutdown();
    }

    bool VideoPlayer::Initialize(HWND hwnd)
    {
        m_hwnd = hwnd;
        return true;
    }

    bool VideoPlayer::LoadVideo(const std::wstring &url)
    {
        // Fecha vídeo anterior se houver
        Shutdown();

        if (!CreateMediaSource(url))
            return false;

        if (!CreateSession())
            return false;

        if (!CreateTopology())
            return false;

        return true;
    }

    bool VideoPlayer::CreateSession()
    {
        HRESULT hr = MFCreateMediaSession(nullptr, &m_pSession);
        if (FAILED(hr))
            return false;

        // Registra para receber eventos
        hr = m_pSession->BeginGetEvent(this, nullptr);
        return SUCCEEDED(hr);
    }

    bool VideoPlayer::CreateMediaSource(const std::wstring &url)
    {
        IMFSourceResolver *pResolver = nullptr;
        MF_OBJECT_TYPE objectType = MF_OBJECT_INVALID;
        IUnknown *pSource = nullptr;

        HRESULT hr = MFCreateSourceResolver(&pResolver);
        if (FAILED(hr))
            return false;

        // Determina se é URL ou arquivo local
        DWORD flags = MF_RESOLUTION_MEDIASOURCE;
        if (url.find(L"http://") == 0 || url.find(L"https://") == 0)
        {
            flags |= MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE;
        }

        hr = pResolver->CreateObjectFromURL(
            url.c_str(),
            flags,
            nullptr,
            &objectType,
            &pSource);

        if (pResolver)
            pResolver->Release();

        if (FAILED(hr))
            return false;

        hr = pSource->QueryInterface(IID_PPV_ARGS(&m_pSource));
        pSource->Release();

        return SUCCEEDED(hr);
    }

    bool VideoPlayer::CreateTopology()
    {
        if (!m_pSession || !m_pSource)
            return false;

        IMFTopology *pTopology = nullptr;
        IMFPresentationDescriptor *pPD = nullptr;
        DWORD cStreams = 0;

        HRESULT hr = MFCreateTopology(&pTopology);
        if (FAILED(hr))
            return false;

        hr = m_pSource->CreatePresentationDescriptor(&pPD);
        if (FAILED(hr))
        {
            pTopology->Release();
            return false;
        }

        hr = pPD->GetStreamDescriptorCount(&cStreams);
        if (FAILED(hr))
        {
            pPD->Release();
            pTopology->Release();
            return false;
        }

        // Adiciona cada stream à topologia
        for (DWORD i = 0; i < cStreams; i++)
        {
            BOOL selected = FALSE;
            IMFStreamDescriptor *pSD = nullptr;
            IMFMediaTypeHandler *pHandler = nullptr;
            GUID majorType;

            hr = pPD->GetStreamDescriptorByIndex(i, &selected, &pSD);
            if (FAILED(hr))
                continue;

            if (!selected)
            {
                pSD->Release();
                continue;
            }

            hr = pSD->GetMediaTypeHandler(&pHandler);
            if (SUCCEEDED(hr))
            {
                hr = pHandler->GetMajorType(&majorType);
                pHandler->Release();
            }

            if (FAILED(hr))
            {
                pSD->Release();
                continue;
            }

            // Cria nós da topologia para vídeo e áudio
            IMFTopologyNode *pSourceNode = nullptr;
            IMFTopologyNode *pOutputNode = nullptr;

            // Nó fonte
            hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pSourceNode);
            if (SUCCEEDED(hr))
            {
                hr = pSourceNode->SetUnknown(MF_TOPONODE_SOURCE, m_pSource);
                if (SUCCEEDED(hr))
                    hr = pSourceNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pPD);
                if (SUCCEEDED(hr))
                    hr = pSourceNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pSD);
                if (SUCCEEDED(hr))
                    hr = pTopology->AddNode(pSourceNode);
            }

            // Nó de saída
            if (SUCCEEDED(hr))
            {
                hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pOutputNode);
            }

            if (SUCCEEDED(hr))
            {
                IMFActivate *pActivate = nullptr;

                if (IsEqualGUID(majorType, MFMediaType_Video))
                {
                    // Cria renderer de vídeo
                    hr = MFCreateVideoRendererActivate(m_hwnd, &pActivate);
                }
                else if (IsEqualGUID(majorType, MFMediaType_Audio))
                {
                    // Cria renderer de áudio
                    hr = MFCreateAudioRendererActivate(&pActivate);
                }

                if (SUCCEEDED(hr) && pActivate)
                {
                    hr = pOutputNode->SetObject(pActivate);
                    pActivate->Release();
                }

                if (SUCCEEDED(hr))
                    hr = pTopology->AddNode(pOutputNode);
            }

            // Conecta os nós
            if (SUCCEEDED(hr) && pSourceNode && pOutputNode)
            {
                hr = pSourceNode->ConnectOutput(0, pOutputNode, 0);
            }

            if (pSourceNode)
                pSourceNode->Release();
            if (pOutputNode)
                pOutputNode->Release();
            pSD->Release();
        }

        // Define a topologia na sessão
        hr = m_pSession->SetTopology(0, pTopology);

        pPD->Release();
        pTopology->Release();

        return SUCCEEDED(hr);
    }

    bool VideoPlayer::Play()
    {
        if (!m_pSession)
            return false;

        PROPVARIANT var;
        PropVariantInit(&var);

        HRESULT hr = m_pSession->Start(&GUID_NULL, &var);

        PropVariantClear(&var);

        if (SUCCEEDED(hr))
        {
            m_isPlaying = true;
            m_isPaused = false;
        }

        return SUCCEEDED(hr);
    }

    bool VideoPlayer::Pause()
    {
        if (!m_pSession)
            return false;

        HRESULT hr = m_pSession->Pause();

        if (SUCCEEDED(hr))
        {
            m_isPlaying = false;
            m_isPaused = true;
        }

        return SUCCEEDED(hr);
    }

    bool VideoPlayer::Stop()
    {
        if (!m_pSession)
            return false;

        HRESULT hr = m_pSession->Stop();

        if (SUCCEEDED(hr))
        {
            m_isPlaying = false;
            m_isPaused = false;
        }

        return SUCCEEDED(hr);
    }

    bool VideoPlayer::TogglePlayPause()
    {
        if (m_isPlaying)
            return Pause();
        else
            return Play();
    }

    void VideoPlayer::SetMuted(bool muted)
    {
        m_muted = muted;

        if (!m_pSession)
            return;

        // Obtém o controle de volume
        IMFSimpleAudioVolume *pVolume = nullptr;
        HRESULT hr = MFGetService(m_pSession, MR_POLICY_VOLUME_SERVICE, IID_PPV_ARGS(&pVolume));

        if (SUCCEEDED(hr))
        {
            pVolume->SetMute(muted);
            pVolume->Release();
        }
    }

    void VideoPlayer::SetVolume(float volume)
    {
        if (!m_pSession)
            return;

        IMFSimpleAudioVolume *pVolume = nullptr;
        HRESULT hr = MFGetService(m_pSession, MR_POLICY_VOLUME_SERVICE, IID_PPV_ARGS(&pVolume));

        if (SUCCEEDED(hr))
        {
            pVolume->SetMasterVolume(volume);
            pVolume->Release();
        }
    }

    void VideoPlayer::ResizeVideo(UINT width, UINT height)
    {
        if (!m_pSession)
            return;

        // Obtém o controle de display do vídeo
        IMFVideoDisplayControl *pDisplay = nullptr;
        HRESULT hr = MFGetService(m_pSession, MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pDisplay));

        if (SUCCEEDED(hr))
        {
            RECT rcDest = {0, 0, (LONG)width, (LONG)height};
            pDisplay->SetVideoPosition(nullptr, &rcDest);
            pDisplay->Release();
        }
    }

    void VideoPlayer::Shutdown()
    {
        if (m_pSession)
        {
            m_pSession->Close();
            m_pSession->Shutdown();
            m_pSession->Release();
            m_pSession = nullptr;
        }

        if (m_pSource)
        {
            m_pSource->Shutdown();
            m_pSource->Release();
            m_pSource = nullptr;
        }

        if (m_pVideoDisplay)
        {
            m_pVideoDisplay->Release();
            m_pVideoDisplay = nullptr;
        }

        m_isPlaying = false;
        m_isPaused = false;
    }

    // IMFAsyncCallback
    STDMETHODIMP VideoPlayer::QueryInterface(REFIID riid, void **ppv)
    {
        if (!ppv)
            return E_POINTER;

        if (riid == IID_IUnknown || riid == IID_IMFAsyncCallback)
        {
            *ppv = static_cast<IMFAsyncCallback *>(this);
            AddRef();
            return S_OK;
        }

        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG)
    VideoPlayer::AddRef()
    {
        return InterlockedIncrement(&m_refCount);
    }

    STDMETHODIMP_(ULONG)
    VideoPlayer::Release()
    {
        ULONG count = InterlockedDecrement(&m_refCount);
        if (count == 0)
        {
            delete this;
        }
        return count;
    }

    STDMETHODIMP VideoPlayer::Invoke(IMFAsyncResult *pAsyncResult)
    {
        if (!m_pSession)
            return S_OK;

        IMFMediaEvent *pEvent = nullptr;
        HRESULT hr = m_pSession->EndGetEvent(pAsyncResult, &pEvent);

        if (SUCCEEDED(hr))
        {
            HandleSessionEvent(pEvent);
            pEvent->Release();
        }

        // Continua escutando eventos
        if (m_pSession)
        {
            m_pSession->BeginGetEvent(this, nullptr);
        }

        return S_OK;
    }

    void VideoPlayer::HandleSessionEvent(IMFMediaEvent *pEvent)
    {
        MediaEventType eventType;
        HRESULT hr = pEvent->GetType(&eventType);

        if (FAILED(hr))
            return;

        switch (eventType)
        {
        case MESessionTopologySet:
            // Topologia configurada, aplica mute se necessário
            if (m_muted)
            {
                SetMuted(true);
            }
            break;

        case MESessionStarted:
            m_isPlaying = true;
            m_isPaused = false;
            break;

        case MESessionPaused:
            m_isPlaying = false;
            m_isPaused = true;
            break;

        case MESessionStopped:
            m_isPlaying = false;
            m_isPaused = false;
            break;

        case MEEndOfPresentation:
            // Fim do vídeo - reinicia se loop estiver habilitado
            if (m_loop)
            {
                PROPVARIANT var;
                PropVariantInit(&var);
                var.vt = VT_I8;
                var.hVal.QuadPart = 0; // Volta ao início
                m_pSession->Start(&GUID_NULL, &var);
                PropVariantClear(&var);
            }
            else
            {
                m_isPlaying = false;
                m_isPaused = false;
            }
            break;

        case MESessionClosed:
            // Sessão fechada
            break;
        }
    }

    bool VideoPlayer::CaptureCurrentFrame(BYTE **ppData, UINT32 *pWidth, UINT32 *pHeight)
    {
        if (!m_pSession)
            return false;

        // Obtém o controle de display do vídeo
        IMFVideoDisplayControl *pDisplay = nullptr;
        HRESULT hr = MFGetService(m_pSession, MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pDisplay));

        if (FAILED(hr))
            return false;

        BITMAPINFOHEADER bih = {};
        bih.biSize = sizeof(bih);
        BYTE *pBits = nullptr;
        DWORD cbBits = 0;

        hr = pDisplay->GetCurrentImage(&bih, &pBits, &cbBits, nullptr);
        pDisplay->Release();

        if (SUCCEEDED(hr) && pBits && cbBits > 0)
        {
            *ppData = pBits; // Caller deve liberar com CoTaskMemFree
            *pWidth = bih.biWidth;
            *pHeight = abs(bih.biHeight);
            return true;
        }

        return false;
    }

} // namespace autopatch
