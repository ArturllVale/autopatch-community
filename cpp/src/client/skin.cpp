#include <Windows.h>
#include <objidl.h>
#include "skin.h"
#include "../core/utils.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#pragma comment(lib, "gdiplus.lib")

namespace autopatch
{

    // Resource IDs (devem corresponder ao builder)
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

    // Singleton
    static Skin *g_skin = nullptr;

    Skin &GetSkin()
    {
        if (!g_skin)
        {
            g_skin = new Skin();
        }
        return *g_skin;
    }

    Skin::Skin()
    {
        // Inicializa GDI+
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);

        m_fontCollection = new Gdiplus::PrivateFontCollection();
    }

    Skin::~Skin()
    {
        // Libera imagens
        m_backgroundImage.reset();
        m_buttonNormalImage.reset();
        m_buttonHoverImage.reset();
        m_buttonPressedImage.reset();

        // Libera font collection
        delete m_fontCollection;

        // Finaliza GDI+
        if (m_gdiplusToken)
        {
            Gdiplus::GdiplusShutdown(m_gdiplusToken);
        }
    }

    bool Skin::LoadFromResources()
    {
        // Carrega configuração
        auto configData = LoadRcData(ID_CONFIG);
        if (configData.empty())
        {
            // Sem config embutida - usa config padrão ou mostra erro
            return false;
        }

        if (!ParseConfig(configData))
        {
            return false;
        }

        // Carrega informações do skin
        auto skinInfoData = LoadRcData(ID_SKIN_DATA);
        if (!skinInfoData.empty())
        {
            ParseSkinInfo(skinInfoData);
        }

        // Carrega imagens para Image Mode
        if (m_config.uiType == UIType::Image)
        {
            auto bgData = LoadRcData(ID_BACKGROUND);
            if (!bgData.empty())
            {
                m_backgroundImage = CreateImageFromBytes(bgData);
            }

            auto btnNormalData = LoadRcData(ID_BUTTON_NORMAL);
            if (!btnNormalData.empty())
            {
                m_buttonNormalImage = CreateImageFromBytes(btnNormalData);
            }

            auto btnHoverData = LoadRcData(ID_BUTTON_HOVER);
            if (!btnHoverData.empty())
            {
                m_buttonHoverImage = CreateImageFromBytes(btnHoverData);
            }

            auto btnPressedData = LoadRcData(ID_BUTTON_PRESSED);
            if (!btnPressedData.empty())
            {
                m_buttonPressedImage = CreateImageFromBytes(btnPressedData);
            }

            // Carrega fonte customizada
            auto fontData = LoadRcData(ID_CUSTOM_FONT);
            if (!fontData.empty())
            {
                LoadCustomFont(fontData);
            }
        }
        // Carrega conteúdo HTML para HTML Mode
        else if (m_config.uiType == UIType::Html)
        {
            auto htmlData = LoadRcData(ID_HTML_CONTENT);
            if (!htmlData.empty())
            {
                m_htmlContent = std::string(htmlData.begin(), htmlData.end());
            }

            auto cssData = LoadRcData(ID_CSS_CONTENT);
            if (!cssData.empty())
            {
                m_cssContent = std::string(cssData.begin(), cssData.end());
            }

            auto jsData = LoadRcData(ID_JS_CONTENT);
            if (!jsData.empty())
            {
                m_jsContent = std::string(jsData.begin(), jsData.end());
            }
        }

        m_loaded = true;
        return true;
    }

    std::vector<uint8_t> Skin::LoadRcData(int resourceId)
    {
        HRSRC hResource = FindResourceW(nullptr, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
        if (!hResource)
        {
            return {};
        }

        HGLOBAL hMemory = ::LoadResource(nullptr, hResource);
        if (!hMemory)
        {
            return {};
        }

        DWORD size = SizeofResource(nullptr, hResource);
        void *data = LockResource(hMemory);

        if (!data || size == 0)
        {
            return {};
        }

        std::vector<uint8_t> result(size);
        memcpy(result.data(), data, size);

        return result;
    }

    std::unique_ptr<Gdiplus::Image> Skin::CreateImageFromBytes(const std::vector<uint8_t> &data)
    {
        if (data.empty())
        {
            return nullptr;
        }

        // Cria IStream a partir dos dados
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, data.size());
        if (!hMem)
        {
            return nullptr;
        }

        void *pMem = GlobalLock(hMem);
        memcpy(pMem, data.data(), data.size());
        GlobalUnlock(hMem);

        IStream *pStream = nullptr;
        if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) != S_OK)
        {
            GlobalFree(hMem);
            return nullptr;
        }

        // Cria imagem
        auto image = std::make_unique<Gdiplus::Image>(pStream);
        pStream->Release();

        if (image->GetLastStatus() != Gdiplus::Ok)
        {
            return nullptr;
        }

        return image;
    }

    bool Skin::LoadCustomFont(const std::vector<uint8_t> &fontData)
    {
        if (fontData.empty() || !m_fontCollection)
        {
            return false;
        }

        // Adiciona fonte à coleção
        Gdiplus::Status status = m_fontCollection->AddMemoryFont(fontData.data(),
                                                                 static_cast<INT>(fontData.size()));

        if (status != Gdiplus::Ok)
        {
            return false;
        }

        // Obtém nome da fonte
        INT found = 0;
        INT count = m_fontCollection->GetFamilyCount();

        if (count > 0)
        {
            Gdiplus::FontFamily *families = new Gdiplus::FontFamily[count];
            m_fontCollection->GetFamilies(count, families, &found);

            if (found > 0)
            {
                wchar_t familyName[LF_FACESIZE];
                families[0].GetFamilyName(familyName);
                m_customFontName = familyName;
            }

            delete[] families;
        }

        // Também instala a fonte no sistema para esta sessão
        DWORD numFonts = 0;
        AddFontMemResourceEx(const_cast<uint8_t *>(fontData.data()),
                             static_cast<DWORD>(fontData.size()), nullptr, &numFonts);

        return !m_customFontName.empty();
    }

    bool Skin::ParseConfig(const std::vector<uint8_t> &jsonData)
    {
        try
        {
            std::string jsonStr(jsonData.begin(), jsonData.end());
            json j = json::parse(jsonStr);

            m_config.serverName = j.value("serverName", "");
            m_config.patchListUrl = j.value("patchListUrl", "");
            m_config.newsUrl = j.value("newsUrl", "");
            m_config.clientExe = j.value("clientExe", "ragexe.exe");
            m_config.clientArgs = j.value("clientArgs", "");

            if (j.contains("grfFiles") && j["grfFiles"].is_array())
            {
                for (const auto &grf : j["grfFiles"])
                {
                    m_config.grfFiles.push_back(grf.get<std::string>());
                }
            }

            // UI Type - suporta tanto o formato antigo (uiType: number) quanto o novo (uiMode: string)
            if (j.contains("uiMode") && j["uiMode"].is_string())
            {
                std::string uiMode = j["uiMode"].get<std::string>();
                if (uiMode == "html")
                {
                    m_config.uiType = UIType::Html;
                }
                else
                {
                    m_config.uiType = UIType::Image; // default to image
                }
            }
            else
            {
                m_config.uiType = static_cast<UIType>(j.value("uiType", 0));
            }

            m_config.windowWidth = j.value("windowWidth", 800);
            m_config.windowHeight = j.value("windowHeight", 600);
            m_config.allowResize = j.value("allowResize", false);
            m_config.showInTaskbar = j.value("showInTaskbar", true);

            // Novo formato do Vue Builder - elements array
            if (j.contains("elements") && j["elements"].is_array())
            {
                m_config.imageMode = std::make_shared<ImageModeConfig>();

                for (const auto &elem : j["elements"])
                {
                    std::string type = elem.value("type", "");

                    if (type == "button")
                    {
                        ButtonConfig btnConfig;
                        btnConfig.id = elem.value("id", "");
                        btnConfig.action = elem.value("action", "");
                        btnConfig.x = elem.value("x", 0);
                        btnConfig.y = elem.value("y", 0);
                        btnConfig.width = elem.value("width", 100);
                        btnConfig.height = elem.value("height", 30);
                        btnConfig.text = elem.value("text", "");
                        m_config.imageMode->buttons.push_back(btnConfig);
                    }
                    else if (type == "label")
                    {
                        LabelConfig lblConfig;
                        lblConfig.id = elem.value("id", "");
                        lblConfig.x = elem.value("x", 0);
                        lblConfig.y = elem.value("y", 0);
                        lblConfig.width = elem.value("width", 200);
                        lblConfig.height = elem.value("height", 20);
                        lblConfig.text = elem.value("text", "");
                        lblConfig.fontSize = elem.value("fontSize", 12);
                        lblConfig.fontColor = elem.value("fontColor", "#FFFFFF");
                        m_config.imageMode->labels.push_back(lblConfig);
                    }
                    else if (type == "status")
                    {
                        LabelConfig lblConfig;
                        lblConfig.id = elem.value("id", "");
                        lblConfig.x = elem.value("x", 0);
                        lblConfig.y = elem.value("y", 0);
                        lblConfig.width = elem.value("width", 200);
                        lblConfig.height = elem.value("height", 20);
                        lblConfig.text = elem.value("text", "Status");
                        lblConfig.fontSize = elem.value("fontSize", 12);
                        lblConfig.fontColor = elem.value("fontColor", "#00FF00");
                        lblConfig.isStatusLabel = true;
                        m_config.imageMode->labels.push_back(lblConfig);
                    }
                    else if (type == "percentage")
                    {
                        LabelConfig lblConfig;
                        lblConfig.id = elem.value("id", "");
                        lblConfig.x = elem.value("x", 0);
                        lblConfig.y = elem.value("y", 0);
                        lblConfig.width = elem.value("width", 100);
                        lblConfig.height = elem.value("height", 20);
                        lblConfig.text = elem.value("text", "0%");
                        lblConfig.fontSize = elem.value("fontSize", 12);
                        lblConfig.fontColor = elem.value("fontColor", "#FFCC00");
                        lblConfig.isPercentageLabel = true;
                        m_config.imageMode->labels.push_back(lblConfig);
                    }
                }
            }

            // Novo formato - progressBar object
            if (j.contains("progressBar") && j["progressBar"].is_object())
            {
                if (!m_config.imageMode)
                {
                    m_config.imageMode = std::make_shared<ImageModeConfig>();
                }

                const auto &pb = j["progressBar"];
                m_config.imageMode->progressBar.x = pb.value("x", 50);
                m_config.imageMode->progressBar.y = pb.value("y", 550);
                m_config.imageMode->progressBar.width = pb.value("width", 600);
                m_config.imageMode->progressBar.height = pb.value("height", 20);
                m_config.imageMode->progressBar.backgroundColor = pb.value("backgroundColor", "#333333");
                m_config.imageMode->progressBar.fillColor = pb.value("fillColor", "#00FF00");
                m_config.imageMode->progressBar.borderColor = pb.value("borderColor", "#666666");
            }

            // Image mode config - formato antigo (para compatibilidade)
            if (j.contains("imageMode") && j["imageMode"].is_object())
            {
                m_config.imageMode = std::make_shared<ImageModeConfig>();

                if (j["imageMode"].contains("buttons") && j["imageMode"]["buttons"].is_array())
                {
                    for (const auto &btn : j["imageMode"]["buttons"])
                    {
                        ButtonConfig btnConfig;
                        btnConfig.id = btn.value("id", "");
                        btnConfig.action = btn.value("action", "");
                        btnConfig.x = btn.value("x", 0);
                        btnConfig.y = btn.value("y", 0);
                        btnConfig.width = btn.value("width", 100);
                        btnConfig.height = btn.value("height", 30);
                        btnConfig.text = btn.value("text", "");
                        m_config.imageMode->buttons.push_back(btnConfig);
                    }
                }

                if (j["imageMode"].contains("labels") && j["imageMode"]["labels"].is_array())
                {
                    for (const auto &lbl : j["imageMode"]["labels"])
                    {
                        LabelConfig lblConfig;
                        lblConfig.id = lbl.value("id", "");
                        lblConfig.x = lbl.value("x", 0);
                        lblConfig.y = lbl.value("y", 0);
                        lblConfig.width = lbl.value("width", 200);
                        lblConfig.height = lbl.value("height", 20);
                        lblConfig.text = lbl.value("text", "");
                        lblConfig.fontSize = lbl.value("fontSize", 12);
                        lblConfig.fontColor = lbl.value("fontColor", "#FFFFFF");
                        lblConfig.textAlign = static_cast<TextAlignment>(lbl.value("alignment", 0));
                        m_config.imageMode->labels.push_back(lblConfig);
                    }
                }

                if (j["imageMode"].contains("progressBar") && j["imageMode"]["progressBar"].is_object())
                {
                    const auto &pb = j["imageMode"]["progressBar"];
                    m_config.imageMode->progressBar.x = pb.value("x", 0);
                    m_config.imageMode->progressBar.y = pb.value("y", 0);
                    m_config.imageMode->progressBar.width = pb.value("width", 400);
                    m_config.imageMode->progressBar.height = pb.value("height", 20);
                    m_config.imageMode->progressBar.backgroundColor = pb.value("backgroundColor", "#333333");
                    m_config.imageMode->progressBar.fillColor = pb.value("fillColor", "#00FF00");
                    m_config.imageMode->progressBar.borderColor = pb.value("borderColor", "#666666");
                }
            }

            // HTML mode config
            if (j.contains("htmlMode") && j["htmlMode"].is_object())
            {
                m_config.htmlMode = std::make_shared<HtmlModeConfig>();
                m_config.htmlMode->startButtonId = j["htmlMode"].value("startButtonId", "btn-start");
                m_config.htmlMode->progressBarId = j["htmlMode"].value("progressBarId", "progress-bar");
                m_config.htmlMode->statusLabelId = j["htmlMode"].value("statusLabelId", "status-text");
                m_config.htmlMode->closeButtonId = j["htmlMode"].value("closeButtonId", "btn-close");
                m_config.htmlMode->minimizeButtonId = j["htmlMode"].value("minimizeButtonId", "btn-minimize");
            }

            return true;
        }
        catch (const std::exception &)
        {
            return false;
        }
    }

    bool Skin::ParseSkinInfo(const std::vector<uint8_t> &jsonData)
    {
        try
        {
            std::string jsonStr(jsonData.begin(), jsonData.end());
            json j = json::parse(jsonStr);

            // Por enquanto apenas para informação
            // Os flags indicam quais recursos estão presentes

            return true;
        }
        catch (const std::exception &)
        {
            return false;
        }
    }

} // namespace autopatch
