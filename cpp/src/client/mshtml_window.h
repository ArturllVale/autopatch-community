// mshtml_window.h - MSHTML (IE) HTML Window
// Fallback for systems without WebView2 (Windows 7/8/10)
// Uses the Internet Explorer WebBrowser control

#pragma once

#include <Windows.h>
#include <functional>
#include <string>

namespace autopatch
{

    class MshtmlWindow
    {
    public:
        using StartCallback = std::function<void()>;
        using CloseCallback = std::function<void()>;

        MshtmlWindow();
        ~MshtmlWindow();

        // Non-copyable
        MshtmlWindow(const MshtmlWindow &) = delete;
        MshtmlWindow &operator=(const MshtmlWindow &) = delete;

        // Creates the window with embedded WebBrowser control
        bool Create(HINSTANCE hInstance, int width, int height, const std::wstring &title);

        // Loads HTML content
        bool LoadContent(const std::string &html, const std::string &css, const std::string &js);

        // Set progress (calls JS function)
        void SetProgress(int percent, const std::wstring &status);

        // Enable/disable start button (calls JS function)
        void EnableStartButton(bool enabled);

        // Set callbacks
        void SetStartGameCallback(StartCallback callback) { m_startCallback = std::move(callback); }
        void SetCloseCallback(CloseCallback callback) { m_closeCallback = std::move(callback); }

        // Run message loop
        int Run();

        // Get window handle
        HWND GetHwnd() const { return m_hwnd; }

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        bool CreateWebBrowser();
        void ResizeWebBrowser();
        std::wstring BuildFullHtml(const std::string &html, const std::string &css, const std::string &js);

        HWND m_hwnd = nullptr;
        int m_width = 640;
        int m_height = 480;
        std::wstring m_title;

        // COM interfaces
        class WebBrowserImpl;
        WebBrowserImpl *m_pImpl = nullptr;

        StartCallback m_startCallback;
        CloseCallback m_closeCallback;
    };

} // namespace autopatch
