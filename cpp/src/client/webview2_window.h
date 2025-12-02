// webview2_window.h - WebView2 HTML Window
// Uses Microsoft Edge WebView2 control to render HTML/CSS/JS

#pragma once

#include <Windows.h>
#include <functional>
#include <string>
#include <memory>

// Forward declarations
struct ICoreWebView2;
struct ICoreWebView2Controller;
struct ICoreWebView2Environment;

namespace autopatch
{

    // Forward declare handlers
    class EnvironmentCompletedHandler;
    class ControllerCompletedHandler;
    class WebMessageReceivedHandler;

    // Check if WebView2 runtime is available
    bool IsWebView2Available();

    class WebView2Window
    {
    public:
        using StartCallback = std::function<void()>;
        using CloseCallback = std::function<void()>;
        using ProgressCallback = std::function<void(int percent, const std::wstring &status)>;

        WebView2Window();
        ~WebView2Window();

        // Non-copyable
        WebView2Window(const WebView2Window &) = delete;
        WebView2Window &operator=(const WebView2Window &) = delete;

        // Creates the window
        bool Create(HINSTANCE hInstance, int width, int height, const std::wstring &title);

        // Loads HTML content (combines HTML, CSS, JS)
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

        // Friends for callback handlers
        friend class EnvironmentCompletedHandler;
        friend class ControllerCompletedHandler;
        friend class WebMessageReceivedHandler;

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        void InitializeWebView2();
        void OnWebView2Ready();
        void OnWebMessageReceived(const std::wstring &message);
        std::wstring BuildFullHtml(const std::string &html, const std::string &css, const std::string &js);

        HWND m_hwnd = nullptr;
        int m_width = 640;
        int m_height = 480;
        std::wstring m_title;

        ICoreWebView2 *m_webView = nullptr;
        ICoreWebView2Controller *m_controller = nullptr;
        ICoreWebView2Environment *m_environment = nullptr;

        std::wstring m_pendingHtml;
        bool m_webViewReady = false;

        StartCallback m_startCallback;
        CloseCallback m_closeCallback;
    };

} // namespace autopatch
