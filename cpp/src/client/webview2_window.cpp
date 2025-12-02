// webview2_window.cpp - WebView2 HTML Window Implementation

#include <Windows.h>
#include <wrl.h>

#include "webview2_window.h"
#include "../core/utils.h"
#include <WebView2.h>
#include <sstream>
#include <vector>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

using namespace Microsoft::WRL;

namespace autopatch
{

    // Check if WebView2 runtime is available
    bool IsWebView2Available()
    {
        // Try to get the version of WebView2
        LPWSTR versionInfo = nullptr;
        HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &versionInfo);

        if (SUCCEEDED(hr) && versionInfo != nullptr)
        {
            CoTaskMemFree(versionInfo);
            return true;
        }

        return false;
    }

    // Class name for WebView2 window
    static const wchar_t *WEBVIEW2_WINDOW_CLASS = L"AutoPatcherWebView2Window";

    // Handler classes for WebView2 callbacks
    class EnvironmentCompletedHandler : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
    {
    public:
        EnvironmentCompletedHandler(WebView2Window *owner) : m_owner(owner), m_refCount(1) {}

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override
        {
            if (riid == IID_IUnknown || riid == IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)
            {
                *ppvObject = this;
                AddRef();
                return S_OK;
            }
            *ppvObject = nullptr;
            return E_NOINTERFACE;
        }

        ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
        ULONG STDMETHODCALLTYPE Release() override
        {
            auto count = InterlockedDecrement(&m_refCount);
            if (count == 0)
                delete this;
            return count;
        }

        HRESULT STDMETHODCALLTYPE Invoke(HRESULT errorCode, ICoreWebView2Environment *environment) override;

        WebView2Window *m_owner;
        volatile LONG m_refCount;
    };

    class ControllerCompletedHandler : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
    {
    public:
        ControllerCompletedHandler(WebView2Window *owner) : m_owner(owner), m_refCount(1) {}

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override
        {
            if (riid == IID_IUnknown || riid == IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler)
            {
                *ppvObject = this;
                AddRef();
                return S_OK;
            }
            *ppvObject = nullptr;
            return E_NOINTERFACE;
        }

        ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
        ULONG STDMETHODCALLTYPE Release() override
        {
            auto count = InterlockedDecrement(&m_refCount);
            if (count == 0)
                delete this;
            return count;
        }

        HRESULT STDMETHODCALLTYPE Invoke(HRESULT errorCode, ICoreWebView2Controller *controller) override;

        WebView2Window *m_owner;
        volatile LONG m_refCount;
    };

    class WebMessageReceivedHandler : public ICoreWebView2WebMessageReceivedEventHandler
    {
    public:
        WebMessageReceivedHandler(WebView2Window *owner) : m_owner(owner), m_refCount(1) {}

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override
        {
            if (riid == IID_IUnknown || riid == IID_ICoreWebView2WebMessageReceivedEventHandler)
            {
                *ppvObject = this;
                AddRef();
                return S_OK;
            }
            *ppvObject = nullptr;
            return E_NOINTERFACE;
        }

        ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
        ULONG STDMETHODCALLTYPE Release() override
        {
            auto count = InterlockedDecrement(&m_refCount);
            if (count == 0)
                delete this;
            return count;
        }

        HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2 *sender, ICoreWebView2WebMessageReceivedEventArgs *args) override
        {
            LPWSTR message = nullptr;
            args->TryGetWebMessageAsString(&message);
            if (message)
            {
                m_owner->OnWebMessageReceived(message);
                CoTaskMemFree(message);
            }
            return S_OK;
        }

        WebView2Window *m_owner;
        volatile LONG m_refCount;
    };

    // WebView2Window implementation
    WebView2Window::WebView2Window() = default;

    WebView2Window::~WebView2Window()
    {
        if (m_controller)
        {
            m_controller->Close();
            m_controller->Release();
        }
        if (m_webView)
        {
            m_webView->Release();
        }
        if (m_environment)
        {
            m_environment->Release();
        }
    }

    bool WebView2Window::Create(HINSTANCE hInstance, int width, int height, const std::wstring &title)
    {
        m_width = width;
        m_height = height;
        m_title = title;

        // Register window class
        WNDCLASSEXW wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = hInstance;
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszClassName = WEBVIEW2_WINDOW_CLASS;

        RegisterClassExW(&wcex);

        // Calculate window size for client area
        RECT rect = {0, 0, width, height};
        AdjustWindowRect(&rect, WS_POPUP | WS_SYSMENU, FALSE);

        // Center window
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int x = (screenWidth - (rect.right - rect.left)) / 2;
        int y = (screenHeight - (rect.bottom - rect.top)) / 2;

        // Create borderless popup window
        m_hwnd = CreateWindowExW(
            WS_EX_APPWINDOW,
            WEBVIEW2_WINDOW_CLASS,
            title.c_str(),
            WS_POPUP | WS_VISIBLE,
            x, y,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr,
            nullptr,
            hInstance,
            this);

        if (!m_hwnd)
        {
            return false;
        }

        // Initialize WebView2
        InitializeWebView2();

        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);

        return true;
    }

    void WebView2Window::InitializeWebView2()
    {
        // Create WebView2 environment
        auto handler = new EnvironmentCompletedHandler(this);
        HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
            nullptr, nullptr, nullptr, handler);
        handler->Release();

        if (FAILED(hr))
        {
            MessageBoxW(m_hwnd, L"WebView2 não disponível. Edge WebView2 Runtime não instalado.",
                        L"Erro", MB_OK | MB_ICONERROR);
        }
    }

    void WebView2Window::OnWebView2Ready()
    {
        m_webViewReady = true;

        // Apply pending HTML if any
        if (!m_pendingHtml.empty())
        {
            m_webView->NavigateToString(m_pendingHtml.c_str());
            m_pendingHtml.clear();
        }
    }

    void WebView2Window::OnWebMessageReceived(const std::wstring &message)
    {
        // Parse messages from JavaScript
        if (message == L"start")
        {
            if (m_startCallback)
            {
                m_startCallback();
            }
        }
        else if (message == L"close")
        {
            if (m_closeCallback)
            {
                m_closeCallback();
            }
            PostMessage(m_hwnd, WM_CLOSE, 0, 0);
        }
    }

    bool WebView2Window::LoadContent(const std::string &html, const std::string &css, const std::string &js)
    {
        std::wstring fullHtml = BuildFullHtml(html, css, js);

        if (m_webViewReady && m_webView)
        {
            m_webView->NavigateToString(fullHtml.c_str());
        }
        else
        {
            m_pendingHtml = std::move(fullHtml);
        }

        return true;
    }

    std::wstring WebView2Window::BuildFullHtml(const std::string &html, const std::string &css, const std::string &js)
    {
        // Build complete HTML document
        std::wostringstream oss;

        // Check if HTML already has doctype/html structure
        bool hasDoctype = html.find("<!DOCTYPE") != std::string::npos ||
                          html.find("<!doctype") != std::string::npos;
        bool hasHtmlTag = html.find("<html") != std::string::npos;
        bool hasHead = html.find("<head") != std::string::npos;

        if (!hasDoctype)
        {
            oss << L"<!DOCTYPE html>\n";
        }
        if (!hasHtmlTag)
        {
            oss << L"<html>\n";
        }
        if (!hasHead)
        {
            oss << L"<head>\n";
            oss << L"<meta charset=\"UTF-8\">\n";
            oss << L"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
        }

        // Insert CSS
        if (!css.empty())
        {
            oss << L"<style>\n";
            oss << utils::Utf8ToWide(css);
            oss << L"\n</style>\n";
        }

        if (!hasHead)
        {
            oss << L"</head>\n<body>\n";
        }

        // Insert HTML content
        oss << utils::Utf8ToWide(html);
    }

    // Insert JavaScript for host communication
    oss << L"\n<script>\n";
    oss << L"window.patcher = {\n";
    oss << L"  setProgress: function(percent, status) {\n";
    oss << L"    const progressBar = document.getElementById('progress-bar');\n";
    oss << L"    const progressFill = document.getElementById('progress-fill');\n";
    oss << L"    const statusLabel = document.getElementById('status-label');\n";
    oss << L"    const percentLabel = document.getElementById('percent-label');\n";
    oss << L"    if (progressFill) progressFill.style.width = percent + '%';\n";
    oss << L"    if (statusLabel) statusLabel.textContent = status;\n";
    oss << L"    if (percentLabel) percentLabel.textContent = percent + '%';\n";
    oss << L"  },\n";
    oss << L"  enableStartButton: function(enabled) {\n";
    oss << L"    const btn = document.getElementById('btn-start') || document.getElementById('start-button') || document.querySelector('[data-action=\"start\"]') || document.querySelector('.start-button');\n";
    oss << L"    if (btn) {\n";
    oss << L"      btn.disabled = !enabled;\n";
    oss << L"      btn.classList.toggle('disabled', !enabled);\n";
    oss << L"      if (enabled) btn.removeAttribute('disabled'); else btn.setAttribute('disabled', 'disabled');\n";
    oss << L"    }\n";
    oss << L"  },\n";
    oss << L"  start: function() { window.chrome.webview.postMessage('start'); },\n";
    oss << L"  close: function() { window.chrome.webview.postMessage('close'); }\n";
    oss << L"};\n";

    // User's JavaScript
    if (!js.empty())
    {
        oss << utils::Utf8ToWide(js);
    }

    oss << L"\n</script>\n";

    if (!hasHead)
    {
        oss << L"</body>\n</html>";
    }

    return oss.str();
}

void WebView2Window::SetProgress(int percent, const std::wstring &status)
{
    if (!m_webView || !m_webViewReady)
        return;

    // Escape quotes in status
    std::wstring escapedStatus = status;
    size_t pos = 0;
    while ((pos = escapedStatus.find(L"'", pos)) != std::wstring::npos)
    {
        escapedStatus.replace(pos, 1, L"\\'");
        pos += 2;
    }

    // Call JavaScript function
    std::wstring script = L"window.patcher.setProgress(" + std::to_wstring(percent) + L", '" + escapedStatus + L"');";
    m_webView->ExecuteScript(script.c_str(), nullptr);
}

void WebView2Window::EnableStartButton(bool enabled)
{
    if (!m_webView || !m_webViewReady)
        return;

    // Call JavaScript function to enable/disable start button
    std::wstring script = L"window.patcher.enableStartButton(" + std::wstring(enabled ? L"true" : L"false") + L");";
    m_webView->ExecuteScript(script.c_str(), nullptr);
}

int WebView2Window::Run()
{
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WebView2Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WebView2Window *pThis = nullptr;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW *pCreate = reinterpret_cast<CREATESTRUCTW *>(lParam);
        pThis = static_cast<WebView2Window *>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<WebView2Window *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    switch (msg)
    {
    case WM_SIZE:
        if (pThis && pThis->m_controller)
        {
            RECT bounds;
            GetClientRect(hwnd, &bounds);
            pThis->m_controller->put_Bounds(bounds);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_NCHITTEST:
    {
        // Allow window dragging from entire client area
        LRESULT hit = DefWindowProcW(hwnd, msg, wParam, lParam);
        if (hit == HTCLIENT)
        {
            // Check if cursor is in title bar area (top 30px)
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            ScreenToClient(hwnd, &pt);
            if (pt.y < 30)
            {
                return HTCAPTION;
            }
        }
        return hit;
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Callback implementations
HRESULT EnvironmentCompletedHandler::Invoke(HRESULT errorCode, ICoreWebView2Environment *environment)
{
    if (FAILED(errorCode) || !environment)
    {
        return errorCode;
    }

    m_owner->m_environment = environment;
    environment->AddRef();

    auto handler = new ControllerCompletedHandler(m_owner);
    environment->CreateCoreWebView2Controller(m_owner->GetHwnd(), handler);
    handler->Release();

    return S_OK;
}

HRESULT ControllerCompletedHandler::Invoke(HRESULT errorCode, ICoreWebView2Controller *controller)
{
    if (FAILED(errorCode) || !controller)
    {
        return errorCode;
    }

    m_owner->m_controller = controller;
    controller->AddRef();

    controller->get_CoreWebView2(&m_owner->m_webView);

    // Configure WebView2
    if (m_owner->m_webView)
    {
        // Set bounds
        RECT bounds;
        GetClientRect(m_owner->GetHwnd(), &bounds);
        controller->put_Bounds(bounds);

        // Get settings
        ICoreWebView2Settings *settings = nullptr;
        m_owner->m_webView->get_Settings(&settings);
        if (settings)
        {
            settings->put_IsScriptEnabled(TRUE);
            settings->put_AreDefaultScriptDialogsEnabled(FALSE);
            settings->put_IsWebMessageEnabled(TRUE);
            settings->put_AreDevToolsEnabled(FALSE);
            settings->put_IsStatusBarEnabled(FALSE);
            settings->put_AreDefaultContextMenusEnabled(FALSE);
            settings->Release();
        }

        // Register message handler
        EventRegistrationToken token;
        auto msgHandler = new WebMessageReceivedHandler(m_owner);
        m_owner->m_webView->add_WebMessageReceived(msgHandler, &token);
        msgHandler->Release();

        m_owner->OnWebView2Ready();
    }

    return S_OK;
}

} // namespace autopatch
