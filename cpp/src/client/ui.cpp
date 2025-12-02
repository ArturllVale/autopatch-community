#include "ui.h"
#include "../core/config.h"
#include "../core/resources.h"
#include "../core/utils.h"
#include "embedded_browser.h"
#include <sstream>
#include <algorithm>
#include <wincrypt.h>

#pragma comment(lib, "crypt32.lib")

#pragma comment(lib, "gdiplus.lib")

namespace autopatch
{
    using utils::StringToWide;

    // Converte cor hex (#RRGGBB) para Gdiplus::Color
    static Gdiplus::Color ParseHexColor(const std::string &hex)
    {
        if (hex.empty())
            return Gdiplus::Color(0, 0, 0);
        std::string h = hex;
        if (h[0] == '#')
            h = h.substr(1);
        unsigned int val = 0;
        std::stringstream ss;
        ss << std::hex << h;
        ss >> val;
        return Gdiplus::Color(255, (val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF);
    }

    ULONG_PTR UI::s_gdiplusToken = 0;

    UI::UI() = default;

    UI::~UI()
    {
        // Limpa os browsers embarcados
        for (auto &wv : m_webviews)
        {
            if (wv.browser)
            {
                delete wv.browser;
                wv.browser = nullptr;
            }
        }
    }

    bool UI::InitializeGdiPlus()
    {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        return Gdiplus::GdiplusStartup(&s_gdiplusToken, &gdiplusStartupInput, nullptr) == Gdiplus::Ok;
    }

    void UI::ShutdownGdiPlus()
    {
        if (s_gdiplusToken != 0)
        {
            Gdiplus::GdiplusShutdown(s_gdiplusToken);
            s_gdiplusToken = 0;
        }
    }

    bool UI::LoadSkin(const PatcherConfig &config)
    {
        m_windowWidth = config.windowWidth;
        m_windowHeight = config.windowHeight;

        if (config.uiType == UIType::Image && config.imageMode)
        {
            // Primeiro tenta carregar background do recurso embutido
            constexpr int ID_BACKGROUND = 1003;
            auto bgData = Resources::LoadRCData(ID_BACKGROUND);
            if (!bgData.empty())
            {
                m_backgroundImage = LoadImageFromMemory(bgData.data(), bgData.size());
            }
            // Se não encontrou no recurso, tenta Base64 do config
            else if (!config.imageMode->backgroundImage.empty())
            {
                m_backgroundImage = LoadImageFromBase64(StringToWide(config.imageMode->backgroundImage));
            }

            // Carrega boxes (primeiro para ficar no fundo)
            for (const auto &box : config.imageMode->boxes)
            {
                AddBox(box);
            }

            // Carrega imagens
            for (const auto &img : config.imageMode->images)
            {
                AddImage(img);
            }

            // Carrega botões
            for (const auto &btn : config.imageMode->buttons)
            {
                AddButton(btn);
            }

            // Carrega labels
            for (const auto &lbl : config.imageMode->labels)
            {
                AddLabel(lbl);
            }

            // Configura progress bar
            const auto &pb = config.imageMode->progressBar;
            SetProgressBar(pb.x, pb.y, pb.width, pb.height);
            m_progressBar.backgroundColor = ParseHexColor(pb.backgroundColor);
            m_progressBar.fillColor = ParseHexColor(pb.fillColor);

            return true;
        }
        else if (config.uiType == UIType::Html)
        {
            // Modo HTML - Carrega HTML/CSS/JS dos recursos
            // Por enquanto, cria uma UI fallback básica enquanto WebView2 não está implementado
            constexpr int ID_HTML_CONTENT = 1008;
            constexpr int ID_CSS_CONTENT = 1009;
            constexpr int ID_JS_CONTENT = 1010;

            auto htmlData = Resources::LoadRCData(ID_HTML_CONTENT);
            auto cssData = Resources::LoadRCData(ID_CSS_CONTENT);
            auto jsData = Resources::LoadRCData(ID_JS_CONTENT);

            m_htmlContent = std::string(htmlData.begin(), htmlData.end());
            m_cssContent = std::string(cssData.begin(), cssData.end());
            m_jsContent = std::string(jsData.begin(), jsData.end());

            // Cria UI fallback com botão de iniciar e barra de progresso
            // TODO: Implementar WebView2 para renderizar HTML real

            // Adiciona um botão "Iniciar" no centro
            ButtonConfig startBtn;
            startBtn.id = "btn_start";
            startBtn.action = "start_game";
            startBtn.text = "Iniciar Jogo";
            startBtn.x = (m_windowWidth - 150) / 2;
            startBtn.y = m_windowHeight / 2 - 40;
            startBtn.width = 150;
            startBtn.height = 40;
            startBtn.fontName = "Segoe UI";
            startBtn.fontSize = 14;
            startBtn.fontColor = "#ffffff";
            startBtn.backgroundColor = "#0078d4";
            AddButton(startBtn);

            // Adiciona um botão "Fechar" no canto
            ButtonConfig closeBtn;
            closeBtn.id = "btn_close";
            closeBtn.action = "close";
            closeBtn.text = "X";
            closeBtn.x = m_windowWidth - 35;
            closeBtn.y = 5;
            closeBtn.width = 30;
            closeBtn.height = 30;
            closeBtn.fontName = "Segoe UI";
            closeBtn.fontSize = 14;
            closeBtn.fontColor = "#ffffff";
            closeBtn.backgroundColor = "#c42b1c";
            AddButton(closeBtn);

            // Adiciona label de status
            LabelConfig statusLbl;
            statusLbl.id = "lbl_status";
            statusLbl.text = "Modo HTML (WebView2 pendente)";
            statusLbl.x = 20;
            statusLbl.y = m_windowHeight - 70;
            statusLbl.width = m_windowWidth - 40;
            statusLbl.height = 20;
            statusLbl.fontName = "Segoe UI";
            statusLbl.fontSize = 11;
            statusLbl.fontColor = "#cccccc";
            statusLbl.isStatusLabel = true;
            AddLabel(statusLbl);

            // Configura progress bar
            SetProgressBar(20, m_windowHeight - 45, m_windowWidth - 40, 25);
            m_progressBar.backgroundColor = Gdiplus::Color(255, 51, 51, 51);
            m_progressBar.fillColor = Gdiplus::Color(255, 0, 180, 0);

            return true;
        }

        // Modo Image mas sem config.imageMode - criar config padrão
        if (config.uiType == UIType::Image)
        {
            return true;
        }

        return false;
    }

    std::unique_ptr<Gdiplus::Image> UI::LoadImageFromMemory(const void *data, size_t size)
    {
        if (!data || size == 0)
            return nullptr;

        // Cria IStream a partir dos bytes
        IStream *pStream = nullptr;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
        if (!hMem)
            return nullptr;

        void *pMem = GlobalLock(hMem);
        memcpy(pMem, data, size);
        GlobalUnlock(hMem);

        if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) != S_OK)
        {
            GlobalFree(hMem);
            return nullptr;
        }

        auto image = std::make_unique<Gdiplus::Image>(pStream);
        pStream->Release();

        if (image->GetLastStatus() != Gdiplus::Ok)
        {
            return nullptr;
        }

        return image;
    }

    std::unique_ptr<Gdiplus::Image> UI::LoadImageFromBase64(const std::wstring &base64)
    {
        // Decodifica Base64 - converte wstring para string usando a função utilitária
        std::string base64Str = utils::WideToString(base64);

        DWORD decodedSize = 0;
        CryptStringToBinaryA(base64Str.c_str(), 0, CRYPT_STRING_BASE64, nullptr, &decodedSize, nullptr, nullptr);

        if (decodedSize == 0)
            return nullptr;

        std::vector<BYTE> decoded(decodedSize);
        if (!CryptStringToBinaryA(base64Str.c_str(), 0, CRYPT_STRING_BASE64, decoded.data(), &decodedSize, nullptr, nullptr))
        {
            return nullptr;
        }

        // Cria IStream a partir dos bytes
        IStream *pStream = nullptr;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, decodedSize);
        if (!hMem)
            return nullptr;

        void *pMem = GlobalLock(hMem);
        memcpy(pMem, decoded.data(), decodedSize);
        GlobalUnlock(hMem);

        if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) != S_OK)
        {
            GlobalFree(hMem);
            return nullptr;
        }

        auto image = std::make_unique<Gdiplus::Image>(pStream);
        pStream->Release();

        if (image->GetLastStatus() != Gdiplus::Ok)
        {
            return nullptr;
        }

        return image;
    }

    std::unique_ptr<Gdiplus::Image> UI::LoadImageFromResource(int resourceId)
    {
        auto data = Resources::LoadRCData(resourceId);
        if (data.empty())
            return nullptr;

        // Cria IStream
        IStream *pStream = nullptr;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, data.size());
        if (!hMem)
            return nullptr;

        void *pMem = GlobalLock(hMem);
        memcpy(pMem, data.data(), data.size());
        GlobalUnlock(hMem);

        if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) != S_OK)
        {
            GlobalFree(hMem);
            return nullptr;
        }

        auto image = std::make_unique<Gdiplus::Image>(pStream);
        pStream->Release();

        if (image->GetLastStatus() != Gdiplus::Ok)
        {
            return nullptr;
        }

        return image;
    }

    void UI::AddButton(const ButtonConfig &config)
    {
        UIButton button;
        button.id = static_cast<int>(m_buttons.size());
        button.rect = {config.x, config.y, config.x + config.width, config.y + config.height};
        button.action = StringToWide(config.action);
        // Usa text como tooltip para exibição
        button.tooltip = StringToWide(config.text.empty() ? config.tooltip : config.text);

        // Carrega imagens para estados (formato legado)
        if (!config.normalImage.empty())
        {
            button.normalImage = LoadImageFromBase64(StringToWide(config.normalImage));
        }
        if (!config.hoverImage.empty())
        {
            button.hoverImage = LoadImageFromBase64(StringToWide(config.hoverImage));
        }
        if (!config.pressedImage.empty())
        {
            button.pressedImage = LoadImageFromBase64(StringToWide(config.pressedImage));
        }
        if (!config.disabledImage.empty())
        {
            button.disabledImage = LoadImageFromBase64(StringToWide(config.disabledImage));
        }

        // Carrega imagens do novo formato (estados opcionais)
        if (config.normalState && !config.normalState->imagePath.empty())
        {
            button.normalImage = LoadImageFromBase64(StringToWide(config.normalState->imagePath));
        }
        if (config.hoverState && !config.hoverState->imagePath.empty())
        {
            button.hoverImage = LoadImageFromBase64(StringToWide(config.hoverState->imagePath));
        }
        if (config.pressedState && !config.pressedState->imagePath.empty())
        {
            button.pressedImage = LoadImageFromBase64(StringToWide(config.pressedState->imagePath));
        }
        if (config.disabledState && !config.disabledState->imagePath.empty())
        {
            button.disabledImage = LoadImageFromBase64(StringToWide(config.disabledState->imagePath));
        }

        // Cores de estado
        button.normalColor = ParseHexColor(config.backgroundColor);
        if (config.normalState && !config.normalState->backgroundColor.empty())
        {
            button.normalColor = ParseHexColor(config.normalState->backgroundColor);
        }

        if (config.hoverState && !config.hoverState->backgroundColor.empty())
        {
            button.hoverColor = ParseHexColor(config.hoverState->backgroundColor);
        }
        else
        {
            // Cor hover padrão (mais clara)
            button.hoverColor = Gdiplus::Color(255, 0, 140, 210);
        }

        if (config.pressedState && !config.pressedState->backgroundColor.empty())
        {
            button.pressedColor = ParseHexColor(config.pressedState->backgroundColor);
        }
        else
        {
            // Cor pressed padrão (mais escura)
            button.pressedColor = Gdiplus::Color(255, 0, 100, 170);
        }

        if (config.disabledState && !config.disabledState->backgroundColor.empty())
        {
            button.disabledColor = ParseHexColor(config.disabledState->backgroundColor);
        }
        else
        {
            button.disabledColor = Gdiplus::Color(255, 80, 80, 80);
        }

        // Cores de texto por estado
        button.normalTextColor = ParseHexColor(config.fontColor);
        if (config.normalState && !config.normalState->fontColor.empty())
        {
            button.normalTextColor = ParseHexColor(config.normalState->fontColor);
        }

        if (config.hoverState && !config.hoverState->fontColor.empty())
        {
            button.hoverTextColor = ParseHexColor(config.hoverState->fontColor);
        }
        else
        {
            button.hoverTextColor = button.normalTextColor;
        }

        if (config.pressedState && !config.pressedState->fontColor.empty())
        {
            button.pressedTextColor = ParseHexColor(config.pressedState->fontColor);
        }
        else
        {
            button.pressedTextColor = button.normalTextColor;
        }

        if (config.disabledState && !config.disabledState->fontColor.empty())
        {
            button.disabledTextColor = ParseHexColor(config.disabledState->fontColor);
        }
        else
        {
            button.disabledTextColor = Gdiplus::Color(255, 150, 150, 150);
        }

        // Font properties
        button.fontName = StringToWide(config.fontName.empty() ? "Segoe UI" : config.fontName);
        button.fontSize = config.fontSize > 0 ? config.fontSize : 14;
        button.bold = config.fontBold;
        button.italic = config.fontItalic;

        // Border radius
        button.borderRadius = config.effects.borderRadius;

        // Opacity
        button.opacity = config.effects.opacity / 100.0f;

        // Shadow
        button.shadow.enabled = config.effects.shadow.enabled;
        button.shadow.color = ParseHexColor(config.effects.shadow.color);
        button.shadow.offsetX = config.effects.shadow.offsetX;
        button.shadow.offsetY = config.effects.shadow.offsetY;
        button.shadow.blur = config.effects.shadow.blur;

        // Glow
        button.glow.enabled = config.effects.glow.enabled;
        button.glow.color = ParseHexColor(config.effects.glow.color);
        button.glow.intensity = config.effects.glow.intensity / 100.0f;

        m_buttons.push_back(std::move(button));
    }

    void UI::AddLabel(const LabelConfig &config)
    {
        UILabel label;
        label.rect = {config.x, config.y, config.x + config.width, config.y + config.height};
        label.text = StringToWide(config.text);
        label.fontName = StringToWide(config.fontName);
        label.fontSize = config.fontSize;
        label.fontColor = ParseHexColor(config.fontColor);
        label.alignment = static_cast<int>(config.textAlign);

        // IDs especiais baseados no tipo de label
        // ID 1 = status label, ID 2 = percentage label
        if (config.isStatusLabel)
        {
            label.id = 1;
        }
        else if (config.isPercentageLabel)
        {
            label.id = 2;
        }
        else if (!config.id.empty() && (config.id == "1" || config.id == "2"))
        {
            label.id = std::stoi(config.id);
        }
        else
        {
            static int labelCounter = 100;
            label.id = labelCounter++;
        }
        m_labels[label.id] = label;
    }

    void UI::AddLabel(int id, int x, int y, int width, int height,
                      const std::wstring &text, const std::wstring &fontName,
                      int fontSize, uint32_t color, int alignment)
    {
        UILabel label;
        label.id = id;
        label.rect = {x, y, x + width, y + height};
        label.text = text;
        label.fontName = fontName;
        label.fontSize = fontSize;
        label.fontColor = Gdiplus::Color(
            255,
            (color >> 16) & 0xFF,
            (color >> 8) & 0xFF,
            color & 0xFF);
        label.alignment = alignment;

        m_labels[id] = label;
    }

    void UI::AddBox(const BoxConfig &config)
    {
        UIBox box;
        box.id = StringToWide(config.id);
        box.rect = {config.x, config.y, config.x + config.width, config.y + config.height};
        box.backgroundColor = ParseHexColor(config.style.fillColor);
        box.borderColor = ParseHexColor(config.style.borderColor);
        box.borderWidth = config.style.borderWidth;
        box.borderRadius = config.style.borderRadius;
        box.opacity = config.effects.opacity / 100.0f;

        // Shadow
        box.shadow.enabled = config.effects.shadow.enabled;
        box.shadow.color = ParseHexColor(config.effects.shadow.color);
        box.shadow.offsetX = config.effects.shadow.offsetX;
        box.shadow.offsetY = config.effects.shadow.offsetY;
        box.shadow.blur = config.effects.shadow.blur;

        // Glow
        box.glow.enabled = config.effects.glow.enabled;
        box.glow.color = ParseHexColor(config.effects.glow.color);
        box.glow.intensity = config.effects.glow.intensity / 100.0f;

        m_boxes.push_back(std::move(box));
    }

    void UI::AddImage(const ImageConfig &config)
    {
        UIImage image;
        image.id = StringToWide(config.id);
        image.rect = {config.x, config.y, config.x + config.width, config.y + config.height};
        image.opacity = config.effects.opacity / 100.0f;
        image.borderRadius = config.effects.borderRadius;

        // Load image from base64
        if (!config.imagePath.empty())
        {
            image.image = LoadImageFromBase64(StringToWide(config.imagePath));
        }

        // Shadow
        image.shadow.enabled = config.effects.shadow.enabled;
        image.shadow.color = ParseHexColor(config.effects.shadow.color);
        image.shadow.offsetX = config.effects.shadow.offsetX;
        image.shadow.offsetY = config.effects.shadow.offsetY;
        image.shadow.blur = config.effects.shadow.blur;

        // Glow
        image.glow.enabled = config.effects.glow.enabled;
        image.glow.color = ParseHexColor(config.effects.glow.color);
        image.glow.intensity = config.effects.glow.intensity / 100.0f;

        m_images.push_back(std::move(image));
    }

    void UI::AddWebView(const WebViewConfig &config, HWND parentHwnd)
    {
        UIWebView webview;
        webview.id = StringToWide(config.id);
        webview.rect = {config.x, config.y, config.x + config.width, config.y + config.height};
        webview.url = StringToWide(config.url);
        webview.backgroundColor = ParseHexColor(config.backgroundColor);
        webview.borderColor = ParseHexColor(config.borderColor);
        webview.borderWidth = config.borderWidth;
        webview.borderRadius = config.borderRadius;
        webview.visible = config.visible;
        webview.zIndex = config.zIndex;

        // Calcula posição com borda
        int webX = config.x + config.borderWidth;
        int webY = config.y + config.borderWidth;
        int webW = config.width - (config.borderWidth * 2);
        int webH = config.height - (config.borderWidth * 2);

        // Cria WebBrowser embarcado e mantém o objeto vivo
        EmbeddedBrowser *browser = new EmbeddedBrowser();
        if (browser->Create(parentHwnd, webX, webY, webW, webH))
        {
            browser->Navigate(webview.url);
            webview.hwndBrowser = browser->GetHwnd();
            webview.browser = browser; // Armazena para não ser destruido
        }
        else
        {
            delete browser;
            browser = nullptr;
        }

        m_webviews.push_back(std::move(webview));
    }

    void UI::SetProgressBar(int x, int y, int width, int height)
    {
        m_progressBar.rect = {x, y, x + width, y + height};
    }

    bool UI::HasLabel(int id) const
    {
        return m_labels.find(id) != m_labels.end();
    }

    void UI::SetLabelText(int id, const std::wstring &text)
    {
        auto it = m_labels.find(id);
        if (it != m_labels.end())
        {
            it->second.text = text;
        }
    }

    void UI::SetProgress(float progress)
    {
        m_progressBar.progress = std::clamp(progress, 0.0f, 1.0f);
    }

    void UI::EnableButton(const std::wstring &action, bool enabled)
    {
        for (auto &button : m_buttons)
        {
            if (button.action == action)
            {
                button.enabled = enabled;
                button.state = enabled ? ButtonState::Normal : ButtonState::Disabled;
            }
        }
    }

    void UI::BringWebViewsToFront()
    {
        // Traz todas as janelas de WebView para frente e garante visibilidade
        for (const auto &wv : m_webviews)
        {
            if (wv.visible && wv.hwndBrowser)
            {
                // Garante que a janela está visível
                ShowWindow(wv.hwndBrowser, SW_SHOW);
                // Traz para frente sem ativar (não rouba foco)
                SetWindowPos(wv.hwndBrowser, HWND_TOP, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
            }
        }
    }

    std::vector<RECT> UI::GetWebViewRects() const
    {
        std::vector<RECT> rects;
        for (const auto &wv : m_webviews)
        {
            if (wv.visible && wv.hwndBrowser)
            {
                rects.push_back(wv.rect);
            }
        }
        return rects;
    }

    void UI::Render(HDC hdc, int width, int height)
    {
        Gdiplus::Graphics graphics(hdc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

        // Cria região de clipping que exclui as áreas dos WebViews
        // Isso evita que o desenho sobrescreva os controles WebBrowser
        Gdiplus::Region clipRegion(Gdiplus::Rect(0, 0, width, height));
        for (const auto &wv : m_webviews)
        {
            if (wv.visible && wv.hwndBrowser)
            {
                Gdiplus::Rect wvRect(wv.rect.left, wv.rect.top,
                                     wv.rect.right - wv.rect.left,
                                     wv.rect.bottom - wv.rect.top);
                clipRegion.Exclude(wvRect);
            }
        }
        graphics.SetClip(&clipRegion);

        // Desenha background
        if (m_backgroundImage)
        {
            graphics.DrawImage(m_backgroundImage.get(), 0, 0, width, height);
        }
        else
        {
            Gdiplus::SolidBrush brush(Gdiplus::Color(30, 30, 30));
            graphics.FillRectangle(&brush, 0, 0, width, height);
        }

        // Desenha boxes (containers translúcidos)
        for (const auto &box : m_boxes)
        {
            int boxX = box.rect.left;
            int boxY = box.rect.top;
            int boxW = box.rect.right - box.rect.left;
            int boxH = box.rect.bottom - box.rect.top;

            // Calcula alpha baseado na opacidade
            BYTE alpha = static_cast<BYTE>(box.opacity * 255);

            // Desenha sombra se habilitada
            if (box.shadow.enabled)
            {
                BYTE shadowAlpha = static_cast<BYTE>(box.shadow.color.GetA() * box.opacity);
                Gdiplus::Color shadowColor(shadowAlpha, box.shadow.color.GetR(),
                                           box.shadow.color.GetG(), box.shadow.color.GetB());
                Gdiplus::SolidBrush shadowBrush(shadowColor);

                // Desenha sombra com offset
                if (box.borderRadius > 0)
                {
                    Gdiplus::GraphicsPath shadowPath;
                    int sr = box.borderRadius;
                    int sx = boxX + box.shadow.offsetX;
                    int sy = boxY + box.shadow.offsetY;
                    shadowPath.AddArc(sx, sy, sr * 2, sr * 2, 180, 90);
                    shadowPath.AddArc(sx + boxW - sr * 2, sy, sr * 2, sr * 2, 270, 90);
                    shadowPath.AddArc(sx + boxW - sr * 2, sy + boxH - sr * 2, sr * 2, sr * 2, 0, 90);
                    shadowPath.AddArc(sx, sy + boxH - sr * 2, sr * 2, sr * 2, 90, 90);
                    shadowPath.CloseFigure();
                    graphics.FillPath(&shadowBrush, &shadowPath);
                }
                else
                {
                    graphics.FillRectangle(&shadowBrush, boxX + box.shadow.offsetX,
                                           boxY + box.shadow.offsetY, boxW, boxH);
                }
            }

            // Desenha glow se habilitado
            if (box.glow.enabled)
            {
                BYTE glowAlpha = static_cast<BYTE>(50 * box.glow.intensity * box.opacity);
                for (int i = 3; i >= 1; i--)
                {
                    Gdiplus::Color glowColor(glowAlpha / i, box.glow.color.GetR(),
                                             box.glow.color.GetG(), box.glow.color.GetB());
                    Gdiplus::Pen glowPen(glowColor, static_cast<float>(i * 2));
                    if (box.borderRadius > 0)
                    {
                        Gdiplus::GraphicsPath glowPath;
                        int gr = box.borderRadius + i;
                        glowPath.AddArc(boxX - i, boxY - i, gr * 2, gr * 2, 180, 90);
                        glowPath.AddArc(boxX + boxW - gr * 2 - i, boxY - i, gr * 2, gr * 2, 270, 90);
                        glowPath.AddArc(boxX + boxW - gr * 2 - i, boxY + boxH - gr * 2 - i, gr * 2, gr * 2, 0, 90);
                        glowPath.AddArc(boxX - i, boxY + boxH - gr * 2 - i, gr * 2, gr * 2, 90, 90);
                        glowPath.CloseFigure();
                        graphics.DrawPath(&glowPen, &glowPath);
                    }
                    else
                    {
                        graphics.DrawRectangle(&glowPen, boxX - i, boxY - i, boxW + i * 2, boxH + i * 2);
                    }
                }
            }

            // Cor de fundo com alpha
            Gdiplus::Color bgColor(static_cast<BYTE>(box.backgroundColor.GetA() * box.opacity),
                                   box.backgroundColor.GetR(), box.backgroundColor.GetG(),
                                   box.backgroundColor.GetB());
            Gdiplus::SolidBrush bgBrush(bgColor);

            // Desenha box com ou sem border radius
            if (box.borderRadius > 0)
            {
                Gdiplus::GraphicsPath path;
                int r = box.borderRadius;
                path.AddArc(boxX, boxY, r * 2, r * 2, 180, 90);
                path.AddArc(boxX + boxW - r * 2, boxY, r * 2, r * 2, 270, 90);
                path.AddArc(boxX + boxW - r * 2, boxY + boxH - r * 2, r * 2, r * 2, 0, 90);
                path.AddArc(boxX, boxY + boxH - r * 2, r * 2, r * 2, 90, 90);
                path.CloseFigure();
                graphics.FillPath(&bgBrush, &path);

                // Borda
                if (box.borderWidth > 0)
                {
                    Gdiplus::Color borderColor(static_cast<BYTE>(box.borderColor.GetA() * box.opacity),
                                               box.borderColor.GetR(), box.borderColor.GetG(),
                                               box.borderColor.GetB());
                    Gdiplus::Pen borderPen(borderColor, static_cast<float>(box.borderWidth));
                    graphics.DrawPath(&borderPen, &path);
                }
            }
            else
            {
                graphics.FillRectangle(&bgBrush, boxX, boxY, boxW, boxH);

                // Borda
                if (box.borderWidth > 0)
                {
                    Gdiplus::Color borderColor(static_cast<BYTE>(box.borderColor.GetA() * box.opacity),
                                               box.borderColor.GetR(), box.borderColor.GetG(),
                                               box.borderColor.GetB());
                    Gdiplus::Pen borderPen(borderColor, static_cast<float>(box.borderWidth));
                    graphics.DrawRectangle(&borderPen, boxX, boxY, boxW, boxH);
                }
            }
        }

        // Desenha imagens
        for (const auto &img : m_images)
        {
            if (!img.image)
                continue;

            int imgX = img.rect.left;
            int imgY = img.rect.top;
            int imgW = img.rect.right - img.rect.left;
            int imgH = img.rect.bottom - img.rect.top;

            // Desenha sombra se habilitada
            if (img.shadow.enabled)
            {
                BYTE shadowAlpha = static_cast<BYTE>(100 * img.opacity);
                Gdiplus::Color shadowColor(shadowAlpha, img.shadow.color.GetR(),
                                           img.shadow.color.GetG(), img.shadow.color.GetB());
                Gdiplus::SolidBrush shadowBrush(shadowColor);

                if (img.borderRadius > 0)
                {
                    Gdiplus::GraphicsPath shadowPath;
                    int sr = img.borderRadius;
                    int sx = imgX + img.shadow.offsetX;
                    int sy = imgY + img.shadow.offsetY;
                    shadowPath.AddArc(sx, sy, sr * 2, sr * 2, 180, 90);
                    shadowPath.AddArc(sx + imgW - sr * 2, sy, sr * 2, sr * 2, 270, 90);
                    shadowPath.AddArc(sx + imgW - sr * 2, sy + imgH - sr * 2, sr * 2, sr * 2, 0, 90);
                    shadowPath.AddArc(sx, sy + imgH - sr * 2, sr * 2, sr * 2, 90, 90);
                    shadowPath.CloseFigure();
                    graphics.FillPath(&shadowBrush, &shadowPath);
                }
                else
                {
                    graphics.FillRectangle(&shadowBrush, imgX + img.shadow.offsetX,
                                           imgY + img.shadow.offsetY, imgW, imgH);
                }
            }

            // Desenha glow se habilitado
            if (img.glow.enabled)
            {
                BYTE glowAlpha = static_cast<BYTE>(50 * img.glow.intensity * img.opacity);
                for (int i = 3; i >= 1; i--)
                {
                    Gdiplus::Color glowColor(glowAlpha / i, img.glow.color.GetR(),
                                             img.glow.color.GetG(), img.glow.color.GetB());
                    Gdiplus::Pen glowPen(glowColor, static_cast<float>(i * 2));
                    graphics.DrawRectangle(&glowPen, imgX - i, imgY - i, imgW + i * 2, imgH + i * 2);
                }
            }

            // Aplica opacidade usando ColorMatrix
            Gdiplus::ColorMatrix colorMatrix = {
                1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, img.opacity, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
            Gdiplus::ImageAttributes imgAttr;
            imgAttr.SetColorMatrix(&colorMatrix);

            // Desenha imagem com clipping se tiver border radius
            if (img.borderRadius > 0)
            {
                Gdiplus::GraphicsPath clipPath;
                int r = img.borderRadius;
                clipPath.AddArc(imgX, imgY, r * 2, r * 2, 180, 90);
                clipPath.AddArc(imgX + imgW - r * 2, imgY, r * 2, r * 2, 270, 90);
                clipPath.AddArc(imgX + imgW - r * 2, imgY + imgH - r * 2, r * 2, r * 2, 0, 90);
                clipPath.AddArc(imgX, imgY + imgH - r * 2, r * 2, r * 2, 90, 90);
                clipPath.CloseFigure();

                Gdiplus::Region clipRegion(&clipPath);
                graphics.SetClip(&clipRegion);
                graphics.DrawImage(img.image.get(),
                                   Gdiplus::Rect(imgX, imgY, imgW, imgH),
                                   0, 0, img.image->GetWidth(), img.image->GetHeight(),
                                   Gdiplus::UnitPixel, &imgAttr);
                graphics.ResetClip();
            }
            else
            {
                graphics.DrawImage(img.image.get(),
                                   Gdiplus::Rect(imgX, imgY, imgW, imgH),
                                   0, 0, img.image->GetWidth(), img.image->GetHeight(),
                                   Gdiplus::UnitPixel, &imgAttr);
            }
        }

        // Desenha botões
        for (const auto &button : m_buttons)
        {
            int btnX = button.rect.left;
            int btnY = button.rect.top;
            int btnW = button.rect.right - button.rect.left;
            int btnH = button.rect.bottom - button.rect.top;

            // Calcula alpha baseado na opacidade
            BYTE alpha = static_cast<BYTE>(button.opacity * 255);

            // Desenha sombra se habilitada
            if (button.shadow.enabled)
            {
                BYTE shadowAlpha = static_cast<BYTE>(button.shadow.color.GetA() * button.opacity);
                Gdiplus::Color shadowColor(shadowAlpha, button.shadow.color.GetR(),
                                           button.shadow.color.GetG(), button.shadow.color.GetB());
                Gdiplus::SolidBrush shadowBrush(shadowColor);

                if (button.borderRadius > 0)
                {
                    Gdiplus::GraphicsPath shadowPath;
                    int sr = button.borderRadius;
                    int sx = btnX + button.shadow.offsetX;
                    int sy = btnY + button.shadow.offsetY;
                    shadowPath.AddArc(sx, sy, sr * 2, sr * 2, 180, 90);
                    shadowPath.AddArc(sx + btnW - sr * 2, sy, sr * 2, sr * 2, 270, 90);
                    shadowPath.AddArc(sx + btnW - sr * 2, sy + btnH - sr * 2, sr * 2, sr * 2, 0, 90);
                    shadowPath.AddArc(sx, sy + btnH - sr * 2, sr * 2, sr * 2, 90, 90);
                    shadowPath.CloseFigure();
                    graphics.FillPath(&shadowBrush, &shadowPath);
                }
                else
                {
                    graphics.FillRectangle(&shadowBrush, btnX + button.shadow.offsetX,
                                           btnY + button.shadow.offsetY, btnW, btnH);
                }
            }

            // Desenha glow se habilitado (mais intenso no hover)
            if (button.glow.enabled)
            {
                float glowMultiplier = (button.state == ButtonState::Hover) ? 1.5f : 1.0f;
                BYTE glowAlpha = static_cast<BYTE>(50 * button.glow.intensity * button.opacity * glowMultiplier);
                for (int i = 3; i >= 1; i--)
                {
                    Gdiplus::Color glowColor(glowAlpha / i, button.glow.color.GetR(),
                                             button.glow.color.GetG(), button.glow.color.GetB());
                    Gdiplus::Pen glowPen(glowColor, static_cast<float>(i * 2));
                    if (button.borderRadius > 0)
                    {
                        Gdiplus::GraphicsPath glowPath;
                        int gr = button.borderRadius + i;
                        glowPath.AddArc(btnX - i, btnY - i, gr * 2, gr * 2, 180, 90);
                        glowPath.AddArc(btnX + btnW - gr * 2 - i, btnY - i, gr * 2, gr * 2, 270, 90);
                        glowPath.AddArc(btnX + btnW - gr * 2 - i, btnY + btnH - gr * 2 - i, gr * 2, gr * 2, 0, 90);
                        glowPath.AddArc(btnX - i, btnY + btnH - gr * 2 - i, gr * 2, gr * 2, 90, 90);
                        glowPath.CloseFigure();
                        graphics.DrawPath(&glowPen, &glowPath);
                    }
                    else
                    {
                        graphics.DrawRectangle(&glowPen, btnX - i, btnY - i, btnW + i * 2, btnH + i * 2);
                    }
                }
            }

            Gdiplus::Image *img = GetButtonImage(button);
            if (img)
            {
                // Aplica opacidade usando ColorMatrix
                Gdiplus::ColorMatrix colorMatrix = {
                    1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, button.opacity, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
                Gdiplus::ImageAttributes imgAttr;
                imgAttr.SetColorMatrix(&colorMatrix);

                if (button.borderRadius > 0)
                {
                    Gdiplus::GraphicsPath clipPath;
                    int r = button.borderRadius;
                    clipPath.AddArc(btnX, btnY, r * 2, r * 2, 180, 90);
                    clipPath.AddArc(btnX + btnW - r * 2, btnY, r * 2, r * 2, 270, 90);
                    clipPath.AddArc(btnX + btnW - r * 2, btnY + btnH - r * 2, r * 2, r * 2, 0, 90);
                    clipPath.AddArc(btnX, btnY + btnH - r * 2, r * 2, r * 2, 90, 90);
                    clipPath.CloseFigure();

                    Gdiplus::Region clipRegion(&clipPath);
                    graphics.SetClip(&clipRegion);
                    graphics.DrawImage(img,
                                       Gdiplus::Rect(btnX, btnY, btnW, btnH),
                                       0, 0, img->GetWidth(), img->GetHeight(),
                                       Gdiplus::UnitPixel, &imgAttr);
                    graphics.ResetClip();
                }
                else
                {
                    graphics.DrawImage(img,
                                       Gdiplus::Rect(btnX, btnY, btnW, btnH),
                                       0, 0, img->GetWidth(), img->GetHeight(),
                                       Gdiplus::UnitPixel, &imgAttr);
                }
            }
            else
            {
                // Desenha botão com cor baseada no estado
                Gdiplus::Color btnColor;
                Gdiplus::Color textColor;
                switch (button.state)
                {
                case ButtonState::Hover:
                    btnColor = button.hoverColor;
                    textColor = button.hoverTextColor;
                    break;
                case ButtonState::Pressed:
                    btnColor = button.pressedColor;
                    textColor = button.pressedTextColor;
                    break;
                case ButtonState::Disabled:
                    btnColor = button.disabledColor;
                    textColor = button.disabledTextColor;
                    break;
                default:
                    btnColor = button.normalColor;
                    textColor = button.normalTextColor;
                    break;
                }

                // Aplica opacidade à cor
                Gdiplus::Color finalColor(static_cast<BYTE>(btnColor.GetA() * button.opacity),
                                          btnColor.GetR(), btnColor.GetG(), btnColor.GetB());
                Gdiplus::SolidBrush btnBrush(finalColor);

                // Desenha com ou sem border radius
                if (button.borderRadius > 0)
                {
                    Gdiplus::GraphicsPath path;
                    int r = button.borderRadius;
                    path.AddArc(btnX, btnY, r * 2, r * 2, 180, 90);
                    path.AddArc(btnX + btnW - r * 2, btnY, r * 2, r * 2, 270, 90);
                    path.AddArc(btnX + btnW - r * 2, btnY + btnH - r * 2, r * 2, r * 2, 0, 90);
                    path.AddArc(btnX, btnY + btnH - r * 2, r * 2, r * 2, 90, 90);
                    path.CloseFigure();
                    graphics.FillPath(&btnBrush, &path);
                }
                else
                {
                    graphics.FillRectangle(&btnBrush, btnX, btnY, btnW, btnH);
                }

                // Desenha texto do botão
                if (!button.action.empty())
                {
                    std::wstring btnText = button.tooltip.empty() ? button.action : button.tooltip;

                    // Traduz actions comuns
                    if (btnText == L"start_game")
                        btnText = L"Jogar";
                    else if (btnText == L"check_updates")
                        btnText = L"Verificar";
                    else if (btnText == L"exit")
                        btnText = L"Sair";

                    // Determina estilo da fonte
                    int fontStyle = Gdiplus::FontStyleRegular;
                    if (button.bold && button.italic)
                        fontStyle = Gdiplus::FontStyleBoldItalic;
                    else if (button.bold)
                        fontStyle = Gdiplus::FontStyleBold;
                    else if (button.italic)
                        fontStyle = Gdiplus::FontStyleItalic;

                    Gdiplus::Font font(button.fontName.c_str(), static_cast<float>(button.fontSize), fontStyle);

                    // Aplica opacidade à cor do texto
                    Gdiplus::Color finalTextColor(static_cast<BYTE>(textColor.GetA() * button.opacity),
                                                  textColor.GetR(), textColor.GetG(), textColor.GetB());
                    Gdiplus::SolidBrush textBrush(finalTextColor);

                    Gdiplus::StringFormat format;
                    format.SetAlignment(Gdiplus::StringAlignmentCenter);
                    format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

                    Gdiplus::RectF btnRect((float)btnX, (float)btnY, (float)btnW, (float)btnH);
                    graphics.DrawString(btnText.c_str(), -1, &font, btnRect, &format, &textBrush);
                }
            }
        }

        // Desenha labels
        for (const auto &[id, label] : m_labels)
        {
            Gdiplus::Font font(label.fontName.c_str(), static_cast<float>(label.fontSize));
            Gdiplus::SolidBrush brush(label.fontColor);

            Gdiplus::StringFormat format;
            if (label.alignment == 1)
                format.SetAlignment(Gdiplus::StringAlignmentCenter);
            else if (label.alignment == 2)
                format.SetAlignment(Gdiplus::StringAlignmentFar);

            Gdiplus::RectF rect(
                static_cast<float>(label.rect.left),
                static_cast<float>(label.rect.top),
                static_cast<float>(label.rect.right - label.rect.left),
                static_cast<float>(label.rect.bottom - label.rect.top));

            graphics.DrawString(label.text.c_str(), -1, &font, rect, &format, &brush);
        }

        // Desenha progress bar
        const auto &pb = m_progressBar;
        int pbWidth = pb.rect.right - pb.rect.left;
        int pbHeight = pb.rect.bottom - pb.rect.top;

        // Background
        if (pb.backgroundImage)
        {
            graphics.DrawImage(pb.backgroundImage.get(), pb.rect.left, pb.rect.top, pbWidth, pbHeight);
        }
        else
        {
            Gdiplus::SolidBrush bgBrush(pb.backgroundColor);
            graphics.FillRectangle(&bgBrush, pb.rect.left, pb.rect.top, pbWidth, pbHeight);
        }

        // Fill
        int fillWidth = static_cast<int>(pbWidth * pb.progress);
        if (fillWidth > 0)
        {
            if (pb.fillImage)
            {
                graphics.DrawImage(pb.fillImage.get(), pb.rect.left, pb.rect.top, fillWidth, pbHeight);
            }
            else
            {
                Gdiplus::SolidBrush fillBrush(pb.fillColor);
                graphics.FillRectangle(&fillBrush, pb.rect.left, pb.rect.top, fillWidth, pbHeight);
            }
        }
    }

    bool UI::OnMouseMove(int x, int y)
    {
        bool needsRedraw = false;

        for (auto &button : m_buttons)
        {
            if (!button.enabled)
                continue;

            ButtonState newState;
            POINT pt = {x, y};

            if (PtInRect(&button.rect, pt))
            {
                newState = (m_pressedButton == &button) ? ButtonState::Pressed : ButtonState::Hover;
            }
            else
            {
                newState = ButtonState::Normal;
            }

            if (button.state != newState)
            {
                button.state = newState;
                needsRedraw = true;
            }
        }

        return needsRedraw;
    }

    bool UI::OnMouseDown(int x, int y)
    {
        UIButton *button = GetButtonAt(x, y);
        if (button && button->enabled)
        {
            button->state = ButtonState::Pressed;
            m_pressedButton = button;
            return true;
        }
        return false;
    }

    bool UI::OnMouseUp(int x, int y)
    {
        if (m_pressedButton)
        {
            UIButton *button = GetButtonAt(x, y);

            if (button == m_pressedButton && button->enabled)
            {
                button->state = ButtonState::Hover;

                if (m_actionCallback)
                {
                    m_actionCallback(button->action);
                }
            }
            else if (m_pressedButton)
            {
                m_pressedButton->state = ButtonState::Normal;
            }

            m_pressedButton = nullptr;
            return true;
        }
        return false;
    }

    void UI::SetActionCallback(ButtonActionCallback callback)
    {
        m_actionCallback = std::move(callback);
    }

    bool UI::IsInDragRegion(int x, int y) const
    {
        // Considera área de arrasto onde não há botões
        POINT pt = {x, y};

        for (const auto &button : m_buttons)
        {
            if (PtInRect(&button.rect, pt))
            {
                return false;
            }
        }

        // Considera os primeiros 50 pixels como área de título
        return y < 50;
    }

    UIButton *UI::GetButtonAt(int x, int y)
    {
        POINT pt = {x, y};

        for (auto &button : m_buttons)
        {
            if (PtInRect(&button.rect, pt))
            {
                return &button;
            }
        }

        return nullptr;
    }

    Gdiplus::Image *UI::GetButtonImage(const UIButton &button)
    {
        switch (button.state)
        {
        case ButtonState::Hover:
            return button.hoverImage ? button.hoverImage.get() : button.normalImage.get();
        case ButtonState::Pressed:
            return button.pressedImage ? button.pressedImage.get() : button.normalImage.get();
        case ButtonState::Disabled:
            return button.disabledImage ? button.disabledImage.get() : button.normalImage.get();
        default:
            return button.normalImage.get();
        }
    }

} // namespace autopatch
