#pragma once

#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <objbase.h>
#include <gdiplus.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

namespace autopatch
{

    // Video background renderer using Source Reader for frame extraction
    // This allows rendering video frames as a background texture behind other UI elements
    class VideoBackground
    {
    public:
        VideoBackground();
        ~VideoBackground();

        // Initialize with target window
        bool Initialize(HWND hwnd);

        // Load video from file path
        bool LoadVideo(const std::wstring &filePath);

        // Playback controls
        void Play();
        void Pause();
        void Stop();
        void TogglePlayPause();

        // State queries
        bool IsPlaying() const { return m_playing && !m_paused; }
        bool IsPaused() const { return m_paused; }
        bool IsLoaded() const { return m_pReader != nullptr; }

        // Settings
        void SetLoop(bool loop) { m_loop = loop; }
        void SetMuted(bool muted) { m_muted = muted; }

        // Get current frame as GDI+ bitmap for rendering
        // Returns nullptr if no frame available
        // Caller should NOT delete the returned bitmap
        Gdiplus::Bitmap *GetCurrentFrame();

        // Get video dimensions
        UINT32 GetVideoWidth() const { return m_videoWidth; }
        UINT32 GetVideoHeight() const { return m_videoHeight; }

        // Shutdown and release resources
        void Shutdown();

    private:
        void DecoderThread();
        bool ReadNextFrame();
        void CreateBitmapFromSample(IMFSample *pSample);

        HWND m_hwnd = nullptr;
        IMFSourceReader *m_pReader = nullptr;

        // Video info
        UINT32 m_videoWidth = 0;
        UINT32 m_videoHeight = 0;
        UINT32 m_stride = 0;
        LONGLONG m_duration = 0;
        double m_frameRate = 30.0;

        // Current frame
        std::mutex m_frameMutex;
        Gdiplus::Bitmap *m_currentFrame = nullptr;
        BYTE *m_frameBuffer = nullptr;
        size_t m_frameBufferSize = 0;

        // Playback state
        std::atomic<bool> m_playing{false};
        std::atomic<bool> m_paused{false};
        std::atomic<bool> m_loop{true};
        std::atomic<bool> m_muted{true};
        std::atomic<bool> m_stopThread{false};

        // Decoder thread
        std::thread m_decoderThread;
        LONGLONG m_currentTime = 0;
    };

} // namespace autopatch
