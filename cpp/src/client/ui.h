#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <Windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include "embedded_browser.h"

#pragma comment(lib, "gdiplus.lib")

namespace autopatch
{

    // Forward declarations
    struct PatcherConfig;
    struct ButtonConfig;
    struct LabelConfig;
    struct BoxConfig;
    struct ImageConfig;
    struct WebViewConfig;

    // Estado de um botão
    enum class ButtonState
    {
        Normal,
        Hover,
        Pressed,
        Disabled
    };

    // Estrutura de efeito de sombra para UI
    struct UIShadow
    {
        bool enabled = false;
        Gdiplus::Color color{128, 0, 0, 0};
        int offsetX = 2;
        int offsetY = 2;
        int blur = 4;
    };

    // Estrutura de efeito de glow para UI
    struct UIGlow
    {
        bool enabled = false;
        Gdiplus::Color color{255, 0, 120, 215};
        float intensity = 0.5f;
    };

    // Informações de um botão na UI
    struct UIButton
    {
        int id;
        RECT rect;
        ButtonState state = ButtonState::Normal;
        std::wstring action;
        std::wstring tooltip;
        std::wstring text;

        // Imagens para cada estado
        std::unique_ptr<Gdiplus::Image> normalImage;
        std::unique_ptr<Gdiplus::Image> hoverImage;
        std::unique_ptr<Gdiplus::Image> pressedImage;
        std::unique_ptr<Gdiplus::Image> disabledImage;

        // Cores para cada estado (quando não há imagem)
        Gdiplus::Color normalColor{255, 0, 122, 204};
        Gdiplus::Color hoverColor{255, 0, 140, 210};
        Gdiplus::Color pressedColor{255, 0, 100, 170};
        Gdiplus::Color disabledColor{255, 80, 80, 80};

        // Cores de texto por estado
        Gdiplus::Color normalTextColor{255, 255, 255, 255};
        Gdiplus::Color hoverTextColor{255, 255, 255, 255};
        Gdiplus::Color pressedTextColor{255, 255, 255, 255};
        Gdiplus::Color disabledTextColor{255, 150, 150, 150};

        // Estilo de texto
        std::wstring fontName = L"Segoe UI";
        int fontSize = 14;
        bool bold = true;
        bool italic = false;

        // Borda
        int borderRadius = 0;

        // Efeitos
        float opacity = 1.0f;
        UIShadow shadow;
        UIGlow glow;

        bool enabled = true;
        bool visible = true;
    };

    // Informações de um label na UI
    struct UILabel
    {
        int id;
        RECT rect;
        std::wstring text;
        std::wstring fontName;
        int fontSize;
        bool bold = false;
        bool italic = false;
        Gdiplus::Color fontColor;
        int alignment;  // 0=left, 1=center, 2=right
        int vAlignment; // 0=top, 1=middle, 2=bottom

        // Efeitos
        float opacity = 1.0f;
        UIShadow shadow;

        bool visible = true;
    };

    // Box translúcida
    struct UIBox
    {
        std::wstring id;
        RECT rect;

        Gdiplus::Color backgroundColor{128, 0, 0, 0};
        Gdiplus::Color borderColor{255, 128, 128, 128};
        int borderWidth = 0;
        int borderRadius = 0;

        // Efeitos
        float opacity = 1.0f;
        UIShadow shadow;
        UIGlow glow;

        bool visible = true;
    };

    // Imagem decorativa
    struct UIImage
    {
        std::wstring id;
        RECT rect;

        std::unique_ptr<Gdiplus::Image> image;

        float opacity = 1.0f;
        int borderRadius = 0;

        // Efeitos
        UIShadow shadow;
        UIGlow glow;

        bool visible = true;
    };

    // WebView embarcado (iframe para conteúdo externo)
    struct UIWebView
    {
        std::wstring id;
        RECT rect;
        std::wstring url;

        Gdiplus::Color backgroundColor{255, 30, 30, 30};
        Gdiplus::Color borderColor{255, 51, 51, 51};
        int borderWidth = 1;
        int borderRadius = 8;

        HWND hwndBrowser = nullptr;         // Handle da janela do navegador
        EmbeddedBrowser *browser = nullptr; // Objeto do navegador (para manter vivo)
        bool visible = true;
        int zIndex = 3;
    };

    // Barra de progresso
    struct UIProgressBar
    {
        RECT rect;
        float progress = 0.0f; // 0.0 - 1.0
        std::unique_ptr<Gdiplus::Image> backgroundImage;
        std::unique_ptr<Gdiplus::Image> fillImage;
        Gdiplus::Color backgroundColor;
        Gdiplus::Color fillColor;
        Gdiplus::Color borderColor;
        int borderRadius = 0;
    };

    // Callback de ação de botão
    using ButtonActionCallback = std::function<void(const std::wstring &action)>;

    // Classe de gerenciamento de UI
    class UI
    {
    public:
        UI();
        ~UI();

        // Inicializa GDI+
        static bool InitializeGdiPlus();
        static void ShutdownGdiPlus();

        // Carrega a skin baseada na configuração
        bool LoadSkin(const PatcherConfig &config);

        // Carrega imagem de Base64
        std::unique_ptr<Gdiplus::Image> LoadImageFromBase64(const std::wstring &base64);

        // Carrega imagem de memória
        std::unique_ptr<Gdiplus::Image> LoadImageFromMemory(const void *data, size_t size);

        // Carrega imagem de recurso
        std::unique_ptr<Gdiplus::Image> LoadImageFromResource(int resourceId);

        // Adiciona elementos
        void AddButton(const ButtonConfig &config);
        void AddLabel(const LabelConfig &config);
        void AddBox(const BoxConfig &config);
        void AddImage(const ImageConfig &config);
        void AddWebView(const WebViewConfig &config, HWND parentHwnd);

        // Legacy: AddLabel com parâmetros individuais
        void AddLabel(int id, int x, int y, int width, int height,
                      const std::wstring &text, const std::wstring &fontName = L"Segoe UI",
                      int fontSize = 12, uint32_t color = 0xFFFFFF, int alignment = 0);
        void SetProgressBar(int x, int y, int width, int height);

        // Verifica se um label existe
        bool HasLabel(int id) const;

        // Atualiza elementos
        void SetLabelText(int id, const std::wstring &text);
        void SetProgress(float progress);
        void EnableButton(const std::wstring &action, bool enabled);

        // Renderização
        void Render(HDC hdc, int width, int height);

        // WebViews
        void BringWebViewsToFront();

        // Eventos de mouse
        bool OnMouseMove(int x, int y);
        bool OnMouseDown(int x, int y);
        bool OnMouseUp(int x, int y);

        // Callback de ação
        void SetActionCallback(ButtonActionCallback callback);

        // Obtém região de arrasto (para janela sem borda)
        bool IsInDragRegion(int x, int y) const;

        // Tamanho da janela
        int GetWindowWidth() const { return m_windowWidth; }
        int GetWindowHeight() const { return m_windowHeight; }

        // Obtém os retângulos das WebViews visíveis (para clipping no BitBlt)
        std::vector<RECT> GetWebViewRects() const;

    private:
        UIButton *GetButtonAt(int x, int y);
        Gdiplus::Image *GetButtonImage(const UIButton &button);

        // Helpers de renderização
        void RenderBox(Gdiplus::Graphics &g, const UIBox &box);
        void RenderImage(Gdiplus::Graphics &g, const UIImage &img);
        void RenderButton(Gdiplus::Graphics &g, const UIButton &btn);
        void RenderLabel(Gdiplus::Graphics &g, const UILabel &lbl);
        void RenderProgressBar(Gdiplus::Graphics &g);

        // Helper para converter cor hex
        static Gdiplus::Color ParseColor(const std::string &hex, int opacity = 100);

        // Helper para desenhar retângulo arredondado
        static void DrawRoundedRect(Gdiplus::Graphics &g, Gdiplus::Pen *pen,
                                    Gdiplus::Brush *brush, const Gdiplus::RectF &rect,
                                    float radius);

        int m_windowWidth = 800;
        int m_windowHeight = 600;

        std::unique_ptr<Gdiplus::Image> m_backgroundImage;
        std::vector<UIButton> m_buttons;
        std::map<int, UILabel> m_labels;
        std::vector<UIBox> m_boxes;
        std::vector<UIImage> m_images;
        std::vector<UIWebView> m_webviews;
        UIProgressBar m_progressBar;

        ButtonActionCallback m_actionCallback;
        UIButton *m_pressedButton = nullptr;

        // HTML mode content (para futura implementação de WebView2)
        std::string m_htmlContent;
        std::string m_cssContent;
        std::string m_jsContent;

        static ULONG_PTR s_gdiplusToken;
    };

} // namespace autopatch
