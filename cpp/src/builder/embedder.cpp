#include "embedder.h"
#include "../core/utils.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <Windows.h>

using json = nlohmann::json;

namespace autopatch
{

    // Resource IDs (devem corresponder aos do cliente)
    constexpr int ID_CONFIG = 1001;
    constexpr int ID_SKIN_DATA = 1002;
    constexpr int ID_BACKGROUND = 1003;
    constexpr int ID_BUTTON_NORMAL = 1004;
    constexpr int ID_BUTTON_HOVER = 1005;
    constexpr int ID_BUTTON_PRESSED = 1006;
    constexpr int ID_CUSTOM_FONT = 1007;
    constexpr int ID_HTML_CONTENT = 1008;
    constexpr int ID_CSS_CONTENT = 1009;
    constexpr int ID_JS_CONTENT = 1010;

    Embedder::Embedder() = default;
    Embedder::~Embedder() = default;

    void Embedder::SetConfig(const PatcherConfig &config)
    {
        m_config = config;
    }

    void Embedder::SetSkin(const SkinData &skin)
    {
        m_skin = skin;
    }

    bool Embedder::LoadBackgroundImage(const std::wstring &path)
    {
        if (!IsValidImageFormat(path))
        {
            return false;
        }

        m_skin.backgroundImage = LoadImageFile(path);
        m_skin.backgroundFormat = GetImageFormat(path);

        return !m_skin.backgroundImage.empty();
    }

    bool Embedder::LoadButtonImages(const std::wstring &normalPath,
                                    const std::wstring &hoverPath,
                                    const std::wstring &pressedPath)
    {
        if (!normalPath.empty())
        {
            m_skin.buttonNormalImage = LoadImageFile(normalPath);
            if (m_skin.buttonNormalImage.empty())
                return false;
        }

        if (!hoverPath.empty())
        {
            m_skin.buttonHoverImage = LoadImageFile(hoverPath);
        }

        if (!pressedPath.empty())
        {
            m_skin.buttonPressedImage = LoadImageFile(pressedPath);
        }

        return true;
    }

    bool Embedder::LoadIcon(const std::wstring &path)
    {
        m_skin.iconData = utils::ReadAllBytes(path);
        return !m_skin.iconData.empty();
    }

    bool Embedder::LoadFont(const std::wstring &path)
    {
        m_skin.fontData = utils::ReadAllBytes(path);
        m_skin.fontName = utils::WideToUtf8(utils::GetFileName(path));
        return !m_skin.fontData.empty();
    }

    bool Embedder::LoadHtmlFiles(const std::wstring &htmlPath,
                                 const std::wstring &cssPath,
                                 const std::wstring &jsPath)
    {
        if (!htmlPath.empty())
        {
            m_skin.htmlContent = utils::ReadAllText(htmlPath);
            if (m_skin.htmlContent.empty())
                return false;
        }

        if (!cssPath.empty())
        {
            m_skin.cssContent = utils::ReadAllText(cssPath);
        }

        if (!jsPath.empty())
        {
            m_skin.jsContent = utils::ReadAllText(jsPath);
        }

        return true;
    }

    bool Embedder::Validate(std::wstring &errorMessage) const
    {
        // Verifica campos obrigatórios
        if (m_config.serverName.empty())
        {
            errorMessage = L"Server name is required";
            return false;
        }

        if (m_config.patchListUrl.empty())
        {
            errorMessage = L"Patch list URL is required";
            return false;
        }

        if (m_config.clientExe.empty())
        {
            errorMessage = L"Client executable name is required";
            return false;
        }

        // Verifica template
        if (m_templatePath.empty() || !utils::FileExists(m_templatePath))
        {
            errorMessage = L"Client template (AutoPatcher.exe) not found";
            return false;
        }

        // Verifica skin baseado no modo
        if (m_config.uiType == UIType::Image)
        {
            if (m_skin.backgroundImage.empty())
            {
                errorMessage = L"Background image is required for Image mode";
                return false;
            }
        }
        else if (m_config.uiType == UIType::Html)
        {
            if (m_skin.htmlContent.empty())
            {
                errorMessage = L"HTML content is required for HTML mode";
                return false;
            }
        }

        return true;
    }

    BuildResult Embedder::Build(const std::wstring &outputPath, BuildProgressCallback callback)
    {
        BuildResult result;

        // Valida
        std::wstring validationError;
        if (!Validate(validationError))
        {
            result.success = false;
            result.errorMessage = validationError;
            return result;
        }

        if (callback)
            callback(10, L"Copying template...");

        // Copia template
        if (!CopyTemplate(outputPath))
        {
            result.success = false;
            result.errorMessage = L"Failed to copy template file";
            return result;
        }

        if (callback)
            callback(30, L"Embedding configuration...");

        // Embeda config
        if (!EmbedConfig(outputPath))
        {
            utils::DeleteFileW(outputPath);
            result.success = false;
            result.errorMessage = L"Failed to embed configuration";
            return result;
        }

        if (callback)
            callback(50, L"Embedding skin data...");

        // Embeda skin
        if (!EmbedSkin(outputPath))
        {
            utils::DeleteFileW(outputPath);
            result.success = false;
            result.errorMessage = L"Failed to embed skin data";
            return result;
        }

        if (callback)
            callback(70, L"Embedding icon...");

        // Embeda ícone (se houver)
        if (!m_skin.iconData.empty())
        {
            if (!EmbedIcon(outputPath))
            {
                // Ícone é opcional, não falha
            }
        }

        if (callback)
            callback(100, L"Build complete!");

        result.success = true;
        result.outputPath = outputPath;
        result.outputSize = utils::GetFileSize(outputPath);

        return result;
    }

    bool Embedder::CopyTemplate(const std::wstring &destPath)
    {
        // Cria diretório de destino se necessário
        std::wstring dir = utils::GetDirectoryPath(destPath);
        if (!dir.empty() && !utils::DirectoryExists(dir))
        {
            utils::CreateDirectoryRecursive(dir);
        }

        return CopyFileW(m_templatePath.c_str(), destPath.c_str(), FALSE) != 0;
    }

    bool Embedder::EmbedConfig(const std::wstring &exePath)
    {
        std::string configJson = SerializeConfig();

        HANDLE hUpdate = BeginUpdateResourceW(exePath.c_str(), FALSE);
        if (!hUpdate)
        {
            return false;
        }

        BOOL success = UpdateResourceW(
            hUpdate,
            RT_RCDATA,
            MAKEINTRESOURCEW(ID_CONFIG),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            (LPVOID)configJson.data(),
            static_cast<DWORD>(configJson.size()));

        EndUpdateResourceW(hUpdate, !success);
        return success != FALSE;
    }

    bool Embedder::EmbedSkin(const std::wstring &exePath)
    {
        HANDLE hUpdate = BeginUpdateResourceW(exePath.c_str(), FALSE);
        if (!hUpdate)
        {
            return false;
        }

        BOOL success = TRUE;

        // Embeda imagem de fundo
        if (!m_skin.backgroundImage.empty())
        {
            success = success && UpdateResourceW(
                                     hUpdate,
                                     RT_RCDATA,
                                     MAKEINTRESOURCEW(ID_BACKGROUND),
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                     (LPVOID)m_skin.backgroundImage.data(),
                                     static_cast<DWORD>(m_skin.backgroundImage.size()));
        }

        // Embeda imagens de botão
        if (!m_skin.buttonNormalImage.empty())
        {
            success = success && UpdateResourceW(
                                     hUpdate,
                                     RT_RCDATA,
                                     MAKEINTRESOURCEW(ID_BUTTON_NORMAL),
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                     (LPVOID)m_skin.buttonNormalImage.data(),
                                     static_cast<DWORD>(m_skin.buttonNormalImage.size()));
        }

        if (!m_skin.buttonHoverImage.empty())
        {
            success = success && UpdateResourceW(
                                     hUpdate,
                                     RT_RCDATA,
                                     MAKEINTRESOURCEW(ID_BUTTON_HOVER),
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                     (LPVOID)m_skin.buttonHoverImage.data(),
                                     static_cast<DWORD>(m_skin.buttonHoverImage.size()));
        }

        if (!m_skin.buttonPressedImage.empty())
        {
            success = success && UpdateResourceW(
                                     hUpdate,
                                     RT_RCDATA,
                                     MAKEINTRESOURCEW(ID_BUTTON_PRESSED),
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                     (LPVOID)m_skin.buttonPressedImage.data(),
                                     static_cast<DWORD>(m_skin.buttonPressedImage.size()));
        }

        // Embeda fonte customizada
        if (!m_skin.fontData.empty())
        {
            success = success && UpdateResourceW(
                                     hUpdate,
                                     RT_RCDATA,
                                     MAKEINTRESOURCEW(ID_CUSTOM_FONT),
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                     (LPVOID)m_skin.fontData.data(),
                                     static_cast<DWORD>(m_skin.fontData.size()));
        }

        // Embeda conteúdo HTML
        if (!m_skin.htmlContent.empty())
        {
            success = success && UpdateResourceW(
                                     hUpdate,
                                     RT_RCDATA,
                                     MAKEINTRESOURCEW(ID_HTML_CONTENT),
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                     (LPVOID)m_skin.htmlContent.data(),
                                     static_cast<DWORD>(m_skin.htmlContent.size()));
        }

        if (!m_skin.cssContent.empty())
        {
            success = success && UpdateResourceW(
                                     hUpdate,
                                     RT_RCDATA,
                                     MAKEINTRESOURCEW(ID_CSS_CONTENT),
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                     (LPVOID)m_skin.cssContent.data(),
                                     static_cast<DWORD>(m_skin.cssContent.size()));
        }

        if (!m_skin.jsContent.empty())
        {
            success = success && UpdateResourceW(
                                     hUpdate,
                                     RT_RCDATA,
                                     MAKEINTRESOURCEW(ID_JS_CONTENT),
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                     (LPVOID)m_skin.jsContent.data(),
                                     static_cast<DWORD>(m_skin.jsContent.size()));
        }

        // Serializa e embeda informações do skin
        std::string skinJson = SerializeSkin();
        success = success && UpdateResourceW(
                                 hUpdate,
                                 RT_RCDATA,
                                 MAKEINTRESOURCEW(ID_SKIN_DATA),
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                 (LPVOID)skinJson.data(),
                                 static_cast<DWORD>(skinJson.size()));

        EndUpdateResourceW(hUpdate, !success);
        return success != FALSE;
    }

    bool Embedder::EmbedIcon(const std::wstring &exePath)
    {
        // Ícones requerem formato especial - usa API mais complexa
        // Por simplicidade, usar ícone padrão ou ferramenta externa
        // TODO: Implementar substituição de ícone completa
        return true;
    }

    std::string Embedder::SerializeConfig() const
    {
        json j;

        j["serverName"] = m_config.serverName;
        j["patchListUrl"] = m_config.patchListUrl;
        j["newsUrl"] = m_config.newsUrl;
        j["clientExe"] = m_config.clientExe;
        j["clientArgs"] = m_config.clientArgs;
        j["grfFiles"] = m_config.grfFiles;
        j["uiType"] = static_cast<int>(m_config.uiType);
        j["windowWidth"] = m_config.windowWidth;
        j["windowHeight"] = m_config.windowHeight;
        j["allowResize"] = m_config.allowResize;
        j["showInTaskbar"] = m_config.showInTaskbar;

        // Image mode config
        if (m_config.imageMode)
        {
            json imgMode;

            // Buttons
            json buttons = json::array();
            for (const auto &btn : m_config.imageMode->buttons)
            {
                json btnJ;
                btnJ["id"] = btn.id;
                btnJ["action"] = btn.action;
                btnJ["x"] = btn.x;
                btnJ["y"] = btn.y;
                btnJ["width"] = btn.width;
                btnJ["height"] = btn.height;
                btnJ["text"] = btn.text;
                buttons.push_back(btnJ);
            }
            imgMode["buttons"] = buttons;

            // Labels
            json labels = json::array();
            for (const auto &lbl : m_config.imageMode->labels)
            {
                json lblJ;
                lblJ["id"] = lbl.id;
                lblJ["x"] = lbl.x;
                lblJ["y"] = lbl.y;
                lblJ["width"] = lbl.width;
                lblJ["height"] = lbl.height;
                lblJ["text"] = lbl.text;
                lblJ["fontSize"] = lbl.fontSize;
                lblJ["fontColor"] = lbl.fontColor;
                lblJ["alignment"] = static_cast<int>(lbl.textAlign);
                labels.push_back(lblJ);
            }
            imgMode["labels"] = labels;

            // Progress bar
            {
                json pb;
                pb["x"] = m_config.imageMode->progressBar.x;
                pb["y"] = m_config.imageMode->progressBar.y;
                pb["width"] = m_config.imageMode->progressBar.width;
                pb["height"] = m_config.imageMode->progressBar.height;
                pb["backgroundColor"] = m_config.imageMode->progressBar.backgroundColor;
                pb["fillColor"] = m_config.imageMode->progressBar.fillColor;
                pb["borderColor"] = m_config.imageMode->progressBar.borderColor;
                imgMode["progressBar"] = pb;
            }

            j["imageMode"] = imgMode;
        }

        // HTML mode config
        if (m_config.htmlMode)
        {
            json htmlMode;
            htmlMode["startButtonId"] = m_config.htmlMode->startButtonId;
            htmlMode["progressBarId"] = m_config.htmlMode->progressBarId;
            htmlMode["statusLabelId"] = m_config.htmlMode->statusLabelId;
            htmlMode["closeButtonId"] = m_config.htmlMode->closeButtonId;
            htmlMode["minimizeButtonId"] = m_config.htmlMode->minimizeButtonId;
            j["htmlMode"] = htmlMode;
        }

        return j.dump(2);
    }

    std::string Embedder::SerializeSkin() const
    {
        json j;

        j["backgroundFormat"] = m_skin.backgroundFormat;
        j["fontName"] = m_skin.fontName;
        j["hasBackground"] = !m_skin.backgroundImage.empty();
        j["hasButtonNormal"] = !m_skin.buttonNormalImage.empty();
        j["hasButtonHover"] = !m_skin.buttonHoverImage.empty();
        j["hasButtonPressed"] = !m_skin.buttonPressedImage.empty();
        j["hasFont"] = !m_skin.fontData.empty();
        j["hasHtml"] = !m_skin.htmlContent.empty();
        j["hasCss"] = !m_skin.cssContent.empty();
        j["hasJs"] = !m_skin.jsContent.empty();

        return j.dump();
    }

    // Static methods
    std::vector<uint8_t> Embedder::LoadImageFile(const std::wstring &path)
    {
        return utils::ReadAllBytes(path);
    }

    bool Embedder::IsValidImageFormat(const std::wstring &path)
    {
        std::wstring ext = utils::GetFileExtension(path);

        // Converte para lowercase
        for (auto &c : ext)
        {
            c = towlower(c);
        }

        return ext == L".png" || ext == L".jpg" || ext == L".jpeg" || ext == L".bmp" || ext == L".gif";
    }

    std::string Embedder::GetImageFormat(const std::wstring &path)
    {
        std::wstring ext = utils::GetFileExtension(path);

        for (auto &c : ext)
        {
            c = towlower(c);
        }

        if (ext == L".png")
            return "png";
        if (ext == L".jpg" || ext == L".jpeg")
            return "jpg";
        if (ext == L".bmp")
            return "bmp";
        if (ext == L".gif")
            return "gif";

        return "unknown";
    }

} // namespace autopatch
