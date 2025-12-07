#include "video_background.h"
#include <chrono>

namespace autopatch
{

    VideoBackground::VideoBackground()
    {
        // Initialize Media Foundation
        MFStartup(MF_VERSION);
    }

    VideoBackground::~VideoBackground()
    {
        Shutdown();
        MFShutdown();
    }

    bool VideoBackground::Initialize(HWND hwnd)
    {
        m_hwnd = hwnd;
        return true;
    }

    bool VideoBackground::LoadVideo(const std::wstring &filePath)
    {
        Shutdown();

        // Create source reader
        IMFAttributes *pAttributes = nullptr;
        HRESULT hr = MFCreateAttributes(&pAttributes, 1);
        if (FAILED(hr))
            return false;

        // Enable video processing (color conversion)
        hr = pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);

        hr = MFCreateSourceReaderFromURL(filePath.c_str(), pAttributes, &m_pReader);
        pAttributes->Release();

        if (FAILED(hr))
        {
            OutputDebugStringW(L"[VideoBackground] Failed to create source reader\n");
            return false;
        }

        // Configure video output format to RGB32
        IMFMediaType *pType = nullptr;
        hr = MFCreateMediaType(&pType);
        if (SUCCEEDED(hr))
        {
            hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            if (SUCCEEDED(hr))
                hr = pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
            if (SUCCEEDED(hr))
                hr = m_pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pType);
            pType->Release();
        }

        if (FAILED(hr))
        {
            OutputDebugStringW(L"[VideoBackground] Failed to set output format\n");
            m_pReader->Release();
            m_pReader = nullptr;
            return false;
        }

        // Get actual video format
        IMFMediaType *pActualType = nullptr;
        hr = m_pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pActualType);
        if (SUCCEEDED(hr))
        {
            UINT32 width = 0, height = 0;
            MFGetAttributeSize(pActualType, MF_MT_FRAME_SIZE, &width, &height);
            m_videoWidth = width;
            m_videoHeight = height;

            // Get stride
            LONG stride = 0;
            hr = pActualType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32 *)&stride);
            if (FAILED(hr))
            {
                // Calculate stride manually (RGB32 = 4 bytes per pixel)
                stride = width * 4;
            }
            m_stride = abs(stride);

            // Get frame rate
            UINT32 numerator = 0, denominator = 1;
            MFGetAttributeRatio(pActualType, MF_MT_FRAME_RATE, &numerator, &denominator);
            if (numerator > 0 && denominator > 0)
            {
                m_frameRate = (double)numerator / (double)denominator;
            }

            pActualType->Release();

            wchar_t buf[256];
            swprintf_s(buf, L"[VideoBackground] Video: %dx%d @ %.2f fps, stride=%d\n",
                       m_videoWidth, m_videoHeight, m_frameRate, m_stride);
            OutputDebugStringW(buf);
        }

        // Get duration
        PROPVARIANT var;
        PropVariantInit(&var);
        hr = m_pReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var);
        if (SUCCEEDED(hr))
        {
            m_duration = var.hVal.QuadPart;
            PropVariantClear(&var);
        }

        // Allocate frame buffer
        m_frameBufferSize = m_stride * m_videoHeight;
        m_frameBuffer = new BYTE[m_frameBufferSize];

        // Read first frame
        ReadNextFrame();

        return true;
    }

    bool VideoBackground::ReadNextFrame()
    {
        if (!m_pReader)
            return false;

        DWORD streamIndex = 0;
        DWORD flags = 0;
        LONGLONG timestamp = 0;
        IMFSample *pSample = nullptr;

        HRESULT hr = m_pReader->ReadSample(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,
            &streamIndex,
            &flags,
            &timestamp,
            &pSample);

        if (FAILED(hr))
        {
            OutputDebugStringW(L"[VideoBackground] ReadSample failed\n");
            return false;
        }

        if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
        {
            if (m_loop)
            {
                // Seek to beginning
                PROPVARIANT var;
                PropVariantInit(&var);
                var.vt = VT_I8;
                var.hVal.QuadPart = 0;
                m_pReader->SetCurrentPosition(GUID_NULL, var);
                PropVariantClear(&var);
                m_currentTime = 0;

                // Try again
                if (pSample)
                    pSample->Release();
                return ReadNextFrame();
            }
            else
            {
                m_playing = false;
                if (pSample)
                    pSample->Release();
                return false;
            }
        }

        if (pSample)
        {
            m_currentTime = timestamp;
            CreateBitmapFromSample(pSample);
            pSample->Release();
            return true;
        }

        return false;
    }

    void VideoBackground::CreateBitmapFromSample(IMFSample *pSample)
    {
        if (!pSample || !m_frameBuffer)
            return;

        IMFMediaBuffer *pBuffer = nullptr;
        HRESULT hr = pSample->ConvertToContiguousBuffer(&pBuffer);
        if (FAILED(hr))
            return;

        BYTE *pData = nullptr;
        DWORD maxLen = 0, currentLen = 0;
        hr = pBuffer->Lock(&pData, &maxLen, &currentLen);
        if (SUCCEEDED(hr) && pData)
        {
            // Copy frame data directly (top-down)
            std::lock_guard<std::mutex> lock(m_frameMutex);

            // Copy directly without flipping
            memcpy(m_frameBuffer, pData, m_frameBufferSize);

            // Create new bitmap
            if (m_currentFrame)
            {
                delete m_currentFrame;
            }

            m_currentFrame = new Gdiplus::Bitmap(
                m_videoWidth, m_videoHeight, m_stride,
                PixelFormat32bppRGB, m_frameBuffer);

            pBuffer->Unlock();
        }
        pBuffer->Release();
    }

    void VideoBackground::DecoderThread()
    {
        auto frameTime = std::chrono::microseconds((long long)(1000000.0 / m_frameRate));
        auto lastFrame = std::chrono::high_resolution_clock::now();

        while (!m_stopThread)
        {
            if (m_playing && !m_paused)
            {
                auto now = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - lastFrame);

                if (elapsed >= frameTime)
                {
                    if (ReadNextFrame())
                    {
                        // Request window repaint
                        if (m_hwnd)
                        {
                            InvalidateRect(m_hwnd, nullptr, FALSE);
                        }
                    }
                    lastFrame = now;
                }
            }

            // Small sleep to prevent CPU spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void VideoBackground::Play()
    {
        if (!m_pReader)
            return;

        m_paused = false;

        if (!m_playing)
        {
            m_playing = true;
            m_stopThread = false;

            // Start decoder thread if not running
            if (!m_decoderThread.joinable())
            {
                m_decoderThread = std::thread(&VideoBackground::DecoderThread, this);
            }
        }
    }

    void VideoBackground::Pause()
    {
        m_paused = true;
    }

    void VideoBackground::Stop()
    {
        m_playing = false;
        m_paused = false;

        // Seek to beginning
        if (m_pReader)
        {
            PROPVARIANT var;
            PropVariantInit(&var);
            var.vt = VT_I8;
            var.hVal.QuadPart = 0;
            m_pReader->SetCurrentPosition(GUID_NULL, var);
            PropVariantClear(&var);
            m_currentTime = 0;
        }
    }

    void VideoBackground::TogglePlayPause()
    {
        if (m_playing && !m_paused)
        {
            Pause();
        }
        else
        {
            Play();
        }
    }

    Gdiplus::Bitmap *VideoBackground::GetCurrentFrame()
    {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        return m_currentFrame;
    }

    void VideoBackground::Shutdown()
    {
        // Stop thread
        m_stopThread = true;
        m_playing = false;

        if (m_decoderThread.joinable())
        {
            m_decoderThread.join();
        }

        // Release resources
        if (m_pReader)
        {
            m_pReader->Release();
            m_pReader = nullptr;
        }

        std::lock_guard<std::mutex> lock(m_frameMutex);
        if (m_currentFrame)
        {
            delete m_currentFrame;
            m_currentFrame = nullptr;
        }

        if (m_frameBuffer)
        {
            delete[] m_frameBuffer;
            m_frameBuffer = nullptr;
        }

        m_videoWidth = 0;
        m_videoHeight = 0;
        m_stride = 0;
        m_currentTime = 0;
    }

} // namespace autopatch
