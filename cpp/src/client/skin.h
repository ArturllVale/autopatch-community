#pragma once

#include "../core/config.h"
#include <Windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace autopatch
{

    // Classe para gerenciar recursos de skin carregados do EXE
    class Skin
    {
    public:
        Skin();
        ~Skin();

        // Carrega todos os recursos embutidos no EXE
        bool LoadFromResources();

        // Verifica se carregou com sucesso
        bool IsLoaded() const { return m_loaded; }

        // Obtém a configuração
        const PatcherConfig &GetConfig() const { return m_config; }

        // Obtém imagens GDI+
        Gdiplus::Image *GetBackgroundImage() const { return m_backgroundImage.get(); }
        Gdiplus::Image *GetButtonNormalImage() const { return m_buttonNormalImage.get(); }
        Gdiplus::Image *GetButtonHoverImage() const { return m_buttonHoverImage.get(); }
        Gdiplus::Image *GetButtonPressedImage() const { return m_buttonPressedImage.get(); }

        // Obtém fonte customizada
        const std::wstring &GetCustomFontName() const { return m_customFontName; }
        bool HasCustomFont() const { return !m_customFontName.empty(); }

        // Obtém conteúdo HTML
        const std::string &GetHtmlContent() const { return m_htmlContent; }
        const std::string &GetCssContent() const { return m_cssContent; }
        const std::string &GetJsContent() const { return m_jsContent; }

        // Verifica se está em HTML mode
        bool IsHtmlMode() const { return m_config.uiType == UIType::Html; }

    private:
        // Carrega recurso RCDATA
        std::vector<uint8_t> LoadRcData(int resourceId);

        // Cria imagem GDI+ a partir de bytes
        std::unique_ptr<Gdiplus::Image> CreateImageFromBytes(const std::vector<uint8_t> &data);

        // Carrega fonte customizada na memória
        bool LoadCustomFont(const std::vector<uint8_t> &fontData);

        // Parseia configuração JSON
        bool ParseConfig(const std::vector<uint8_t> &jsonData);
        bool ParseSkinInfo(const std::vector<uint8_t> &jsonData);

    private:
        bool m_loaded = false;
        PatcherConfig m_config;

        // Imagens
        std::unique_ptr<Gdiplus::Image> m_backgroundImage;
        std::unique_ptr<Gdiplus::Image> m_buttonNormalImage;
        std::unique_ptr<Gdiplus::Image> m_buttonHoverImage;
        std::unique_ptr<Gdiplus::Image> m_buttonPressedImage;

        // Fonte
        std::wstring m_customFontName;
        Gdiplus::PrivateFontCollection *m_fontCollection = nullptr;

        // HTML content
        std::string m_htmlContent;
        std::string m_cssContent;
        std::string m_jsContent;

        // GDI+ token
        ULONG_PTR m_gdiplusToken = 0;
    };

    // Singleton para acesso global ao skin
    Skin &GetSkin();

} // namespace autopatch
