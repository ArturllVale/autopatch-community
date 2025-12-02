// mshtml_window.cpp - MSHTML (IE) HTML Window Implementation
// Fallback for systems without WebView2
//
// AutoPatch Community
// Copyright (C) 2024 - Cremané (saadrcaa@gmail.com)
// Licensed under MIT License

// IMPORTANT: Include order matters for COM headers
#define _WIN32_WINNT 0x0601 // Windows 7+

#include <Windows.h>
#include <ole2.h>
#include <oleidl.h>
#include <oaidl.h>

// Avoid WIN32_LEAN_AND_MEAN conflicts
#undef WIN32_LEAN_AND_MEAN

#include <exdisp.h>
#include <mshtml.h>
#include <mshtmhst.h>

#include "mshtml_window.h"
#include "../core/utils.h"
#include <sstream>
#include <vector>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

namespace autopatch
{

    // Class name for MSHTML window
    static const wchar_t *MSHTML_WINDOW_CLASS = L"AutoPatcherMshtmlWindow";

// Custom message for JS->C++ communication
#define WM_JS_START (WM_USER + 200)
#define WM_JS_CLOSE (WM_USER + 201)

    // Forward declaration
    class MshtmlWindow::WebBrowserImpl;

    // External object for JavaScript communication (window.external)
    class ExternalDispatch : public IDispatch
    {
    public:
        ExternalDispatch(HWND hwnd) : m_refCount(1), m_hwnd(hwnd) {}

        // IUnknown
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override
        {
            if (riid == IID_IUnknown || riid == IID_IDispatch)
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

        // IDispatch
        HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo) override
        {
            *pctinfo = 0;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT, LCID, ITypeInfo **ppTInfo) override
        {
            *ppTInfo = nullptr;
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID, LPOLESTR *rgszNames, UINT cNames, LCID, DISPID *rgDispId) override
        {
            for (UINT i = 0; i < cNames; i++)
            {
                if (_wcsicmp(rgszNames[i], L"Start") == 0)
                    rgDispId[i] = 1;
                else if (_wcsicmp(rgszNames[i], L"Close") == 0)
                    rgDispId[i] = 2;
                else
                    rgDispId[i] = DISPID_UNKNOWN;
            }
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID, LCID, WORD wFlags,
                                         DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *) override
        {
            if (wFlags & DISPATCH_METHOD)
            {
                switch (dispIdMember)
                {
                case 1: // Start
                    PostMessage(m_hwnd, WM_JS_START, 0, 0);
                    return S_OK;
                case 2: // Close
                    PostMessage(m_hwnd, WM_JS_CLOSE, 0, 0);
                    return S_OK;
                }
            }
            return DISP_E_MEMBERNOTFOUND;
        }

    private:
        volatile LONG m_refCount;
        HWND m_hwnd;
    };

    // IStorage implementation for loading HTML from memory
    class MemoryStorage : public IStorage
    {
    public:
        MemoryStorage() : m_refCount(1) {}

        // IUnknown
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override
        {
            if (riid == IID_IUnknown || riid == IID_IStorage)
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

        // IStorage - all stubs for minimal implementation
        HRESULT STDMETHODCALLTYPE CreateStream(const OLECHAR *, DWORD, DWORD, DWORD, IStream **) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE OpenStream(const OLECHAR *, void *, DWORD, DWORD, IStream **) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE CreateStorage(const OLECHAR *, DWORD, DWORD, DWORD, IStorage **) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE OpenStorage(const OLECHAR *, IStorage *, DWORD, SNB, DWORD, IStorage **) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE CopyTo(DWORD, const IID *, SNB, IStorage *) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE MoveElementTo(const OLECHAR *, IStorage *, const OLECHAR *, DWORD) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE Commit(DWORD) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE Revert() override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE EnumElements(DWORD, void *, DWORD, IEnumSTATSTG **) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE DestroyElement(const OLECHAR *) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE RenameElement(const OLECHAR *, const OLECHAR *) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE SetElementTimes(const OLECHAR *, const FILETIME *, const FILETIME *, const FILETIME *) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE SetClass(REFCLSID) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE SetStateBits(DWORD, DWORD) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE Stat(STATSTG *, DWORD) override { return E_NOTIMPL; }

    private:
        volatile LONG m_refCount;
    };

    // WebBrowser implementation details
    class MshtmlWindow::WebBrowserImpl : public IOleClientSite,
                                         public IOleInPlaceSite,
                                         public IDocHostUIHandler
    {
    public:
        WebBrowserImpl(MshtmlWindow *owner)
            : m_owner(owner), m_refCount(1), m_webBrowser(nullptr), m_oleObject(nullptr), m_external(nullptr) {}

        ~WebBrowserImpl()
        {
            if (m_external)
            {
                m_external->Release();
                m_external = nullptr;
            }
            if (m_webBrowser)
            {
                m_webBrowser->Release();
                m_webBrowser = nullptr;
            }
            if (m_oleObject)
            {
                m_oleObject->Close(OLECLOSE_NOSAVE);
                m_oleObject->Release();
                m_oleObject = nullptr;
            }
        }

        bool Initialize(HWND hwnd)
        {
            m_hwnd = hwnd;

            // Create external dispatch object for JS communication
            m_external = new ExternalDispatch(hwnd);

            // Create WebBrowser control
            HRESULT hr = CoCreateInstance(CLSID_WebBrowser, nullptr, CLSCTX_INPROC_SERVER,
                                          IID_IOleObject, (void **)&m_oleObject);
            if (FAILED(hr))
                return false;

            hr = m_oleObject->SetClientSite(this);
            if (FAILED(hr))
                return false;

            RECT rect;
            GetClientRect(hwnd, &rect);

            hr = m_oleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, nullptr, this, 0, hwnd, &rect);
            if (FAILED(hr))
                return false;

            hr = m_oleObject->QueryInterface(IID_IWebBrowser2, (void **)&m_webBrowser);
            if (FAILED(hr))
                return false;

            // Configure browser
            m_webBrowser->put_Silent(VARIANT_TRUE); // Suppress script errors

            return true;
        }

        void Resize()
        {
            if (!m_oleObject)
                return;

            RECT rect;
            GetClientRect(m_hwnd, &rect);

            IOleInPlaceObject *pInPlace = nullptr;
            if (SUCCEEDED(m_oleObject->QueryInterface(IID_IOleInPlaceObject, (void **)&pInPlace)))
            {
                pInPlace->SetObjectRects(&rect, &rect);
                pInPlace->Release();
            }
        }

        bool NavigateToString(const std::wstring &html)
        {
            if (!m_webBrowser)
            {
                OutputDebugStringW(L"[DEBUG] NavigateToString: m_webBrowser é nullptr!\n");
                return false;
            }

            OutputDebugStringW(L"[DEBUG] NavigateToString: Navegando para about:blank...\n");

            // First navigate to about:blank
            VARIANT vUrl;
            VariantInit(&vUrl);
            vUrl.vt = VT_BSTR;
            vUrl.bstrVal = SysAllocString(L"about:blank");

            VARIANT vEmpty;
            VariantInit(&vEmpty);

            m_webBrowser->Navigate2(&vUrl, &vEmpty, &vEmpty, &vEmpty, &vEmpty);
            VariantClear(&vUrl);

            // Wait for document to be ready
            READYSTATE state;
            int timeout = 100;
            do
            {
                m_webBrowser->get_ReadyState(&state);
                if (state != READYSTATE_COMPLETE)
                {
                    MSG msg;
                    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
                    {
                        // Não processa WM_QUIT aqui
                        if (msg.message == WM_QUIT)
                        {
                            OutputDebugStringW(L"[DEBUG] NavigateToString: WM_QUIT detectado durante carregamento!\n");
                            PostQuitMessage(static_cast<int>(msg.wParam));
                            return false;
                        }
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                    Sleep(10);
                }
            } while (state != READYSTATE_COMPLETE && --timeout > 0);

            OutputDebugStringW(L"[DEBUG] NavigateToString: about:blank carregado, escrevendo HTML...\n");

            // Get document and write HTML
            IDispatch *pDispDoc = nullptr;
            if (SUCCEEDED(m_webBrowser->get_Document(&pDispDoc)) && pDispDoc)
            {
                IHTMLDocument2 *pDoc = nullptr;
                if (SUCCEEDED(pDispDoc->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc)))
                {
                    // Create safe array with HTML
                    SAFEARRAY *psa = SafeArrayCreateVector(VT_VARIANT, 0, 1);
                    if (psa)
                    {
                        VARIANT *pVar;
                        SafeArrayAccessData(psa, (void **)&pVar);
                        pVar->vt = VT_BSTR;
                        pVar->bstrVal = SysAllocString(html.c_str());
                        SafeArrayUnaccessData(psa);

                        pDoc->write(psa);
                        pDoc->close();

                        SafeArrayDestroy(psa);
                    }
                    pDoc->Release();
                }
                pDispDoc->Release();
            }

            return true;
        }

        void ExecuteScript(const std::wstring &script)
        {
            if (!m_webBrowser)
                return;

            IDispatch *pDispDoc = nullptr;
            if (SUCCEEDED(m_webBrowser->get_Document(&pDispDoc)) && pDispDoc)
            {
                IHTMLDocument2 *pDoc = nullptr;
                if (SUCCEEDED(pDispDoc->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc)))
                {
                    IHTMLWindow2 *pWindow = nullptr;
                    if (SUCCEEDED(pDoc->get_parentWindow(&pWindow)))
                    {
                        VARIANT result;
                        VariantInit(&result);
                        BSTR bstrScript = SysAllocString(script.c_str());
                        BSTR bstrLang = SysAllocString(L"JavaScript");
                        pWindow->execScript(bstrScript, bstrLang, &result);
                        SysFreeString(bstrScript);
                        SysFreeString(bstrLang);
                        VariantClear(&result);
                        pWindow->Release();
                    }
                    pDoc->Release();
                }
                pDispDoc->Release();
            }
        }

        // IUnknown
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override
        {
            if (riid == IID_IUnknown || riid == IID_IOleClientSite)
            {
                *ppvObject = static_cast<IOleClientSite *>(this);
                AddRef();
                return S_OK;
            }
            if (riid == IID_IOleInPlaceSite)
            {
                *ppvObject = static_cast<IOleInPlaceSite *>(this);
                AddRef();
                return S_OK;
            }
            if (riid == IID_IDocHostUIHandler)
            {
                *ppvObject = static_cast<IDocHostUIHandler *>(this);
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

        // IOleClientSite
        HRESULT STDMETHODCALLTYPE SaveObject() override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetMoniker(DWORD, DWORD, IMoniker **) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetContainer(IOleContainer **ppContainer) override
        {
            *ppContainer = nullptr;
            return E_NOINTERFACE;
        }
        HRESULT STDMETHODCALLTYPE ShowObject() override { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE RequestNewObjectLayout() override { return E_NOTIMPL; }

        // IOleWindow
        HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd) override
        {
            *phwnd = m_hwnd;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL) override { return E_NOTIMPL; }

        // IOleInPlaceSite
        HRESULT STDMETHODCALLTYPE CanInPlaceActivate() override { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnInPlaceActivate() override { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnUIActivate() override { return S_OK; }
        HRESULT STDMETHODCALLTYPE GetWindowContext(IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc,
                                                   LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo) override
        {
            *ppFrame = nullptr;
            *ppDoc = nullptr;
            GetClientRect(m_hwnd, lprcPosRect);
            GetClientRect(m_hwnd, lprcClipRect);
            lpFrameInfo->fMDIApp = FALSE;
            lpFrameInfo->hwndFrame = m_hwnd;
            lpFrameInfo->haccel = nullptr;
            lpFrameInfo->cAccelEntries = 0;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE Scroll(SIZE) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE OnUIDeactivate(BOOL) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate() override { return S_OK; }
        HRESULT STDMETHODCALLTYPE DiscardUndoState() override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE DeactivateAndUndo() override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE OnPosRectChange(LPCRECT) override { return S_OK; }

        // IDocHostUIHandler
        HRESULT STDMETHODCALLTYPE ShowContextMenu(DWORD, POINT *, IUnknown *, IDispatch *) override { return S_OK; } // Disable context menu
        HRESULT STDMETHODCALLTYPE GetHostInfo(DOCHOSTUIINFO *pInfo) override
        {
            pInfo->cbSize = sizeof(DOCHOSTUIINFO);
            pInfo->dwFlags = DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_SCROLL_NO | DOCHOSTUIFLAG_DISABLE_HELP_MENU;
            pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE ShowUI(DWORD, IOleInPlaceActiveObject *, IOleCommandTarget *, IOleInPlaceFrame *, IOleInPlaceUIWindow *) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE HideUI() override { return S_OK; }
        HRESULT STDMETHODCALLTYPE UpdateUI() override { return S_OK; }
        HRESULT STDMETHODCALLTYPE EnableModeless(BOOL) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnDocWindowActivate(BOOL) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(BOOL) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE ResizeBorder(LPCRECT, IOleInPlaceUIWindow *, BOOL) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG, const GUID *, DWORD) override { return S_FALSE; }
        HRESULT STDMETHODCALLTYPE GetOptionKeyPath(LPOLESTR *, DWORD) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetDropTarget(IDropTarget *, IDropTarget **) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetExternal(IDispatch **ppDispatch) override
        {
            // Return external object for JS->C++ communication (window.external)
            if (m_external)
            {
                m_external->AddRef();
                *ppDispatch = m_external;
                return S_OK;
            }
            *ppDispatch = nullptr;
            return S_FALSE;
        }
        HRESULT STDMETHODCALLTYPE TranslateUrl(DWORD, LPWSTR, LPWSTR *) override { return S_FALSE; }
        HRESULT STDMETHODCALLTYPE FilterDataObject(IDataObject *, IDataObject **) override { return S_FALSE; }

    private:
        MshtmlWindow *m_owner;
        volatile LONG m_refCount;
        HWND m_hwnd = nullptr;
        IWebBrowser2 *m_webBrowser;
        ExternalDispatch *m_external;
        IOleObject *m_oleObject;
    };

    // MshtmlWindow implementation
    MshtmlWindow::MshtmlWindow() = default;

    MshtmlWindow::~MshtmlWindow()
    {
        if (m_pImpl)
        {
            m_pImpl->Release();
            m_pImpl = nullptr;
        }
    }

    bool MshtmlWindow::Create(HINSTANCE hInstance, int width, int height, const std::wstring &title)
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
        wcex.lpszClassName = MSHTML_WINDOW_CLASS;

        RegisterClassExW(&wcex);

        // Calculate window size for client area
        RECT rect = {0, 0, width, height};
        AdjustWindowRect(&rect, WS_POPUP, FALSE);

        // Center window
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int x = (screenWidth - (rect.right - rect.left)) / 2;
        int y = (screenHeight - (rect.bottom - rect.top)) / 2;

        // Create borderless popup window
        m_hwnd = CreateWindowExW(
            WS_EX_APPWINDOW,
            MSHTML_WINDOW_CLASS,
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

        // Create WebBrowser control
        if (!CreateWebBrowser())
        {
            DestroyWindow(m_hwnd);
            return false;
        }

        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);

        return true;
    }

    bool MshtmlWindow::CreateWebBrowser()
    {
        m_pImpl = new WebBrowserImpl(this);
        return m_pImpl->Initialize(m_hwnd);
    }

    void MshtmlWindow::ResizeWebBrowser()
    {
        if (m_pImpl)
        {
            m_pImpl->Resize();
        }
    }

    bool MshtmlWindow::LoadContent(const std::string &html, const std::string &css, const std::string &js)
    {
        if (!m_pImpl)
            return false;

        std::wstring fullHtml = BuildFullHtml(html, css, js);
        return m_pImpl->NavigateToString(fullHtml);
    }

    std::wstring MshtmlWindow::BuildFullHtml(const std::string &html, const std::string &css, const std::string &js)
    {
        std::wostringstream oss;

        // Check if HTML already has structure
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
            // IE compatibility mode
            oss << L"<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\n";
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

        // Insert JavaScript for patcher API
        oss << L"\n<script>\n";
        oss << L"window.patcher = {\n";
        oss << L"  setProgress: function(percent, status) {\n";
        oss << L"    var progressBar = document.getElementById('progress-bar');\n";
        oss << L"    var progressFill = document.getElementById('progress-fill');\n";
        oss << L"    var statusLabel = document.getElementById('status-label') || document.getElementById('status-text');\n";
        oss << L"    var percentLabel = document.getElementById('percent-label') || document.getElementById('progress-percent');\n";
        oss << L"    if (progressFill) progressFill.style.width = percent + '%';\n";
        oss << L"    if (progressBar && !progressFill) progressBar.style.width = percent + '%';\n";
        oss << L"    if (statusLabel) statusLabel.innerText = status;\n";
        oss << L"    if (percentLabel) percentLabel.innerText = percent + '%';\n";
        oss << L"  },\n";
        oss << L"  enableStartButton: function(enabled) {\n";
        oss << L"    var btn = document.getElementById('btn-start') || document.getElementById('start-button') || document.querySelector('[data-action=\"start\"]') || document.querySelector('.start-button');\n";
        oss << L"    if (btn) {\n";
        oss << L"      btn.disabled = !enabled;\n";
        oss << L"      if (enabled) btn.removeAttribute('disabled'); else btn.setAttribute('disabled', 'disabled');\n";
        oss << L"      if (btn.classList) { if (enabled) btn.classList.remove('disabled'); else btn.classList.add('disabled'); }\n";
        oss << L"    }\n";
        oss << L"  },\n";
        oss << L"  start: function() { window.external.Start(); },\n";
        oss << L"  close: function() { window.external.Close(); }\n";
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

    void MshtmlWindow::SetProgress(int percent, const std::wstring &status)
    {
        if (!m_pImpl)
            return;

        // Escape quotes in status
        std::wstring escapedStatus = status;
        size_t pos = 0;
        while ((pos = escapedStatus.find(L"'", pos)) != std::wstring::npos)
        {
            escapedStatus.replace(pos, 1, L"\\'");
            pos += 2;
        }

        std::wstring script = L"if(window.patcher)window.patcher.setProgress(" +
                              std::to_wstring(percent) + L", '" + escapedStatus + L"');";
        m_pImpl->ExecuteScript(script);
    }

    void MshtmlWindow::EnableStartButton(bool enabled)
    {
        if (!m_pImpl)
            return;

        std::wstring script = L"if(window.patcher)window.patcher.enableStartButton(" +
                              std::wstring(enabled ? L"true" : L"false") + L");";
        m_pImpl->ExecuteScript(script);
    }

    int MshtmlWindow::Run()
    {
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return static_cast<int>(msg.wParam);
    }

    LRESULT CALLBACK MshtmlWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MshtmlWindow *pThis = nullptr;

        if (msg == WM_NCCREATE)
        {
            CREATESTRUCTW *pCreate = reinterpret_cast<CREATESTRUCTW *>(lParam);
            pThis = static_cast<MshtmlWindow *>(pCreate->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        }
        else
        {
            pThis = reinterpret_cast<MshtmlWindow *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        switch (msg)
        {
        case WM_SIZE:
            if (pThis)
            {
                pThis->ResizeWebBrowser();
            }
            return 0;

        case WM_DESTROY:
            OutputDebugStringW(L"[DEBUG] WM_DESTROY recebido!\n");
            PostQuitMessage(0);
            return 0;

        case WM_JS_START:
            OutputDebugStringW(L"[DEBUG] WM_JS_START recebido!\n");
            if (pThis && pThis->m_startCallback)
            {
                pThis->m_startCallback();
            }
            return 0;

        case WM_JS_CLOSE:
            OutputDebugStringW(L"[DEBUG] WM_JS_CLOSE recebido!\n");
            if (pThis && pThis->m_closeCallback)
            {
                pThis->m_closeCallback();
            }
            else
            {
                DestroyWindow(hwnd);
            }
            return 0;

        case WM_NCHITTEST:
        {
            // Allow window dragging from title bar area
            LRESULT hit = DefWindowProcW(hwnd, msg, wParam, lParam);
            if (hit == HTCLIENT)
            {
                POINT pt = {LOWORD(lParam), HIWORD(lParam)};
                ScreenToClient(hwnd, &pt);
                if (pt.y < 40)
                {
                    return HTCAPTION;
                }
            }
            return hit;
        }
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

} // namespace autopatch
