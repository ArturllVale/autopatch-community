#pragma once

#include "../core/config.h"
#include "../core/resources.h"
#include <string>
#include <vector>
#include <functional>

namespace autopatch
{

    // Informações do skin/tema para o builder
    struct SkinData
    {
        // Imagem de fundo
        std::vector<uint8_t> backgroundImage;
        std::string backgroundFormat; // png, jpg, bmp

        // Imagens dos botões
        std::vector<uint8_t> buttonNormalImage;
        std::vector<uint8_t> buttonHoverImage;
        std::vector<uint8_t> buttonPressedImage;

        // Fonte customizada (opcional)
        std::vector<uint8_t> fontData;
        std::string fontName;

        // Ícone do EXE
        std::vector<uint8_t> iconData;

        // Para HTML mode
        std::string htmlContent;
        std::string cssContent;
        std::string jsContent;
    };

    // Resultado da construção
    struct BuildResult
    {
        bool success = false;
        std::wstring outputPath;
        std::wstring errorMessage;
        uint64_t outputSize = 0;
    };

    // Callback de progresso
    using BuildProgressCallback = std::function<void(int percent, const std::wstring &status)>;

    // Classe principal do Embedder
    class Embedder
    {
    public:
        Embedder();
        ~Embedder();

        // Configura o patcher
        void SetConfig(const PatcherConfig &config);
        const PatcherConfig &GetConfig() const { return m_config; }

        // Configura o skin
        void SetSkin(const SkinData &skin);
        const SkinData &GetSkin() const { return m_skin; }

        // Carrega imagem de arquivo
        bool LoadBackgroundImage(const std::wstring &path);
        bool LoadButtonImages(const std::wstring &normalPath,
                              const std::wstring &hoverPath = L"",
                              const std::wstring &pressedPath = L"");
        bool LoadIcon(const std::wstring &path);
        bool LoadFont(const std::wstring &path);

        // Para HTML mode
        bool LoadHtmlFiles(const std::wstring &htmlPath,
                           const std::wstring &cssPath = L"",
                           const std::wstring &jsPath = L"");

        // Caminho do cliente base (AutoPatcher.exe template)
        void SetClientTemplatePath(const std::wstring &path) { m_templatePath = path; }
        const std::wstring &GetClientTemplatePath() const { return m_templatePath; }

        // Constrói o patcher final
        BuildResult Build(const std::wstring &outputPath, BuildProgressCallback callback = nullptr);

        // Valida a configuração
        bool Validate(std::wstring &errorMessage) const;

        // Métodos estáticos auxiliares
        static std::vector<uint8_t> LoadImageFile(const std::wstring &path);
        static bool IsValidImageFormat(const std::wstring &path);
        static std::string GetImageFormat(const std::wstring &path);

    private:
        // Copia o template para o destino
        bool CopyTemplate(const std::wstring &destPath);

        // Embeda os recursos no EXE
        bool EmbedConfig(const std::wstring &exePath);
        bool EmbedSkin(const std::wstring &exePath);
        bool EmbedIcon(const std::wstring &exePath);

        // Serializa config e skin para JSON
        std::string SerializeConfig() const;
        std::string SerializeSkin() const;

    private:
        PatcherConfig m_config;
        SkinData m_skin;
        std::wstring m_templatePath;
    };

} // namespace autopatch
