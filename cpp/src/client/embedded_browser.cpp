// embedded_browser.cpp - Simple embedded IE WebBrowser control implementation
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

#undef WIN32_LEAN_AND_MEAN

#include <exdisp.h>
#include <mshtml.h>
#include <mshtmhst.h>

#include "embedded_browser.h"

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

namespace autopatch
{

    // Simple OLE Container for WebBrowser control
    class SimpleOleContainer : public IOleClientSite, public IOleInPlaceSite, public IOleInPlaceFrame
    {
    public:
        SimpleOleContainer(HWND hwnd) : m_refCount(1), m_hwnd(hwnd), m_oleObject(nullptr), m_webBrowser(nullptr) {}

        ~SimpleOleContainer()
        {
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
            if (riid == IID_IOleInPlaceFrame)
            {
                *ppvObject = static_cast<IOleInPlaceFrame *>(this);
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
        HRESULT STDMETHODCALLTYPE GetContainer(IOleContainer **) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE ShowObject() override { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE RequestNewObjectLayout() override { return E_NOTIMPL; }

        // IOleWindow (base for IOleInPlaceSite)
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
                                                   LPRECT lprcPosRect, LPRECT lprcClipRect,
                                                   LPOLEINPLACEFRAMEINFO lpFrameInfo) override
        {
            *ppFrame = static_cast<IOleInPlaceFrame *>(this);
            AddRef();
            *ppDoc = nullptr;
            GetClientRect(m_hwnd, lprcPosRect);
            *lprcClipRect = *lprcPosRect;
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

        // IOleInPlaceFrame
        HRESULT STDMETHODCALLTYPE GetBorder(LPRECT) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE RequestBorderSpace(LPCBORDERWIDTHS) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE SetBorderSpace(LPCBORDERWIDTHS) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE SetActiveObject(IOleInPlaceActiveObject *, LPCOLESTR) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE SetMenu(HMENU, HOLEMENU, HWND) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE RemoveMenus(HMENU) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE SetStatusText(LPCOLESTR) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE EnableModeless(BOOL) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG, WORD) override { return E_NOTIMPL; }

        bool Initialize()
        {
            HRESULT hr = CoCreateInstance(CLSID_WebBrowser, nullptr, CLSCTX_INPROC_SERVER,
                                          IID_IOleObject, (void **)&m_oleObject);
            if (FAILED(hr))
                return false;

            hr = m_oleObject->SetClientSite(this);
            if (FAILED(hr))
                return false;

            RECT rect;
            GetClientRect(m_hwnd, &rect);

            hr = m_oleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, nullptr, this, 0, m_hwnd, &rect);
            if (FAILED(hr))
                return false;

            hr = m_oleObject->QueryInterface(IID_IWebBrowser2, (void **)&m_webBrowser);
            if (FAILED(hr))
                return false;

            m_webBrowser->put_Silent(VARIANT_TRUE);
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

        bool Navigate(const std::wstring &url)
        {
            if (!m_webBrowser)
                return false;

            VARIANT vUrl, vEmpty;
            VariantInit(&vUrl);
            VariantInit(&vEmpty);
            vUrl.vt = VT_BSTR;
            vUrl.bstrVal = SysAllocString(url.c_str());

            HRESULT hr = m_webBrowser->Navigate2(&vUrl, &vEmpty, &vEmpty, &vEmpty, &vEmpty);
            VariantClear(&vUrl);
            return SUCCEEDED(hr);
        }

        IWebBrowser2 *GetBrowser() { return m_webBrowser; }

    private:
        volatile LONG m_refCount;
        HWND m_hwnd;
        IOleObject *m_oleObject;
        IWebBrowser2 *m_webBrowser;
    };

    EmbeddedBrowser::EmbeddedBrowser() : m_hwnd(nullptr), m_pContainer(nullptr)
    {
    }

    EmbeddedBrowser::~EmbeddedBrowser()
    {
        Destroy();
    }

    bool EmbeddedBrowser::Create(HWND parentHwnd, int x, int y, int width, int height)
    {
        // Cria uma janela filha para hospedar o browser
        m_hwnd = CreateWindowExW(
            0,
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
            x, y, width, height,
            parentHwnd,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr);

        if (!m_hwnd)
            return false;

        m_pContainer = new SimpleOleContainer(m_hwnd);
        if (!m_pContainer->Initialize())
        {
            m_pContainer->Release();
            m_pContainer = nullptr;
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
            return false;
        }

        return true;
    }

    bool EmbeddedBrowser::Navigate(const std::wstring &url)
    {
        if (!m_pContainer)
            return false;
        return m_pContainer->Navigate(url);
    }

    void EmbeddedBrowser::Resize(int width, int height)
    {
        if (m_hwnd)
        {
            SetWindowPos(m_hwnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
            if (m_pContainer)
            {
                m_pContainer->Resize();
            }
        }
    }

    void EmbeddedBrowser::Destroy()
    {
        if (m_pContainer)
        {
            m_pContainer->Release();
            m_pContainer = nullptr;
        }
        if (m_hwnd)
        {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }
    }

    // Função helper simplificada
    HWND CreateEmbeddedBrowser(HWND parent, int x, int y, int w, int h, const wchar_t *url)
    {
        EmbeddedBrowser *browser = new EmbeddedBrowser();
        if (!browser->Create(parent, x, y, w, h))
        {
            delete browser;
            return nullptr;
        }

        if (url && wcslen(url) > 0)
        {
            browser->Navigate(url);
        }

        // Armazenar ponteiro no user data da janela para cleanup futuro
        // Por simplicidade, retorna o hwnd e o browser é mantido internamente
        // (em produção, você iria armazenar isso em um mapa ou no user data)
        return browser->GetHwnd();
    }

} // namespace autopatch
