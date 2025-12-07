#include "config.h"
#include <fstream>
#include <sstream>
#include <Windows.h>
#include <nlohmann/json.hpp>

namespace autopatch
{
    using json = nlohmann::json;

    // Resource ID para configuração (deve corresponder ao embedder)
    constexpr int ID_CONFIG = 1001;

    // Extrai configuração embutida no EXE via recursos Win32
    std::string ExtractEmbeddedConfig()
    {
        // Procura recurso embutido
        HRSRC hResource = FindResourceW(nullptr, MAKEINTRESOURCEW(ID_CONFIG), RT_RCDATA);
        if (!hResource)
        {
            return "";
        }

        HGLOBAL hMemory = LoadResource(nullptr, hResource);
        if (!hMemory)
        {
            return "";
        }

        DWORD size = SizeofResource(nullptr, hResource);
        void *data = LockResource(hMemory);
        if (!data || size == 0)
        {
            return "";
        }

        // Converte para string
        return std::string(static_cast<const char *>(data), size);
    }

    // Carrega configuração do JSON
    PatcherConfig LoadConfig()
    {
        PatcherConfig config;

        // Primeiro tenta carregar do EXE embutido
        std::string jsonStr = ExtractEmbeddedConfig();

        // Se não encontrou, tenta carregar de arquivo externo (para desenvolvimento)
        if (jsonStr.empty())
        {
            std::ifstream file("patcher.json");
            if (file.is_open())
            {
                std::stringstream buffer;
                buffer << file.rdbuf();
                jsonStr = buffer.str();
            }
        }

        if (jsonStr.empty())
        {
            // Configuração padrão
            config.serverName = "Meu Servidor RO";
            config.patchListUrl = "http://localhost/patchlist.txt";
            config.windowWidth = 800;
            config.windowHeight = 600;
            config.uiType = UIType::Image;
            config.imageMode = std::make_shared<ImageModeConfig>();
            return config;
        }

        try
        {
            json j = json::parse(jsonStr);

            config.serverName = j.value("serverName", "Meu Servidor");
            config.patchListUrl = j.value("patchListUrl", "");
            config.newsUrl = j.value("newsUrl", "");
            config.clientExe = j.value("clientExe", "ragexe.exe");
            config.clientArgs = j.value("clientArgs", "");
            config.windowWidth = j.value("windowWidth", 800);
            config.windowHeight = j.value("windowHeight", 600);
            config.windowBorderRadius = j.value("windowBorderRadius", 0);

            // Suporta ambos formatos: uiType (número) e uiMode (string)
            if (j.contains("uiMode"))
            {
                std::string mode = j["uiMode"].get<std::string>();
                config.uiType = (mode == "html") ? UIType::Html : UIType::Image;
            }
            else
            {
                config.uiType = static_cast<UIType>(j.value("uiType", 0));
            }

            // GRF files
            if (j.contains("grfFiles") && j["grfFiles"].is_array())
            {
                for (const auto &grf : j["grfFiles"])
                {
                    config.grfFiles.push_back(grf.get<std::string>());
                }
            }

            // NOVO FORMATO: elements array + progressBar (Vue Builder)
            if (j.contains("elements") && j["elements"].is_array())
            {
                config.imageMode = std::make_shared<ImageModeConfig>();

                for (const auto &elem : j["elements"])
                {
                    std::string type = elem.value("type", "");

                    if (type == "button")
                    {
                        ButtonConfig btn;
                        btn.id = elem.value("id", "");
                        btn.action = elem.value("action", "");
                        btn.x = elem.value("x", 0);
                        btn.y = elem.value("y", 0);
                        btn.width = elem.value("width", 100);
                        btn.height = elem.value("height", 30);
                        btn.text = elem.value("text", "");
                        btn.tooltip = elem.value("tooltip", "");

                        // Imagens legadas
                        btn.normalImage = elem.value("normalImage", "");
                        btn.hoverImage = elem.value("hoverImage", "");
                        btn.pressedImage = elem.value("pressedImage", "");
                        btn.disabledImage = elem.value("disabledImage", "");

                        // Fonte
                        btn.fontName = elem.value("fontName", "Segoe UI");
                        btn.fontSize = elem.value("fontSize", 14);
                        btn.fontColor = elem.value("fontColor", "#ffffff");
                        btn.fontBold = elem.value("bold", true);
                        btn.fontItalic = elem.value("italic", false);
                        btn.backgroundColor = elem.value("backgroundColor", "#0078d4");

                        // Estados do botão (novo formato)
                        if (elem.contains("states"))
                        {
                            auto &states = elem["states"];
                            if (states.contains("normal"))
                            {
                                btn.normalState = ElementState();
                                btn.normalState->imagePath = states["normal"].value("imagePath", "");
                                btn.normalState->backgroundColor = states["normal"].value("backgroundColor", "");
                                btn.normalState->fontColor = states["normal"].value("fontColor", "");
                            }
                            if (states.contains("hover"))
                            {
                                btn.hoverState = ElementState();
                                btn.hoverState->imagePath = states["hover"].value("imagePath", "");
                                btn.hoverState->backgroundColor = states["hover"].value("backgroundColor", "");
                                btn.hoverState->fontColor = states["hover"].value("fontColor", "");
                            }
                            if (states.contains("pressed"))
                            {
                                btn.pressedState = ElementState();
                                btn.pressedState->imagePath = states["pressed"].value("imagePath", "");
                                btn.pressedState->backgroundColor = states["pressed"].value("backgroundColor", "");
                                btn.pressedState->fontColor = states["pressed"].value("fontColor", "");
                            }
                            if (states.contains("disabled"))
                            {
                                btn.disabledState = ElementState();
                                btn.disabledState->imagePath = states["disabled"].value("imagePath", "");
                                btn.disabledState->backgroundColor = states["disabled"].value("backgroundColor", "");
                                btn.disabledState->fontColor = states["disabled"].value("fontColor", "");
                            }
                        }

                        // Efeitos - lê do objeto effects se existir
                        if (elem.contains("effects"))
                        {
                            auto &effects = elem["effects"];
                            btn.effects.opacity = effects.value("opacity", 100);
                            btn.effects.borderRadius = effects.value("borderRadius", 0);

                            if (effects.contains("shadow"))
                            {
                                btn.effects.shadow.enabled = effects["shadow"].value("enabled", false);
                                btn.effects.shadow.color = effects["shadow"].value("color", "#000000");
                                btn.effects.shadow.blur = effects["shadow"].value("blur", 4);
                                btn.effects.shadow.offsetX = effects["shadow"].value("offsetX", 2);
                                btn.effects.shadow.offsetY = effects["shadow"].value("offsetY", 2);
                            }

                            if (effects.contains("glow"))
                            {
                                btn.effects.glow.enabled = effects["glow"].value("enabled", false);
                                btn.effects.glow.color = effects["glow"].value("color", "#0078d4");
                                btn.effects.glow.intensity = effects["glow"].value("intensity", 50);
                            }
                        }
                        else
                        {
                            // Fallback para formato antigo
                            btn.effects.opacity = elem.value("opacity", 100);
                            btn.effects.borderRadius = elem.value("borderRadius", 0);
                        }

                        config.imageMode->buttons.push_back(btn);
                    }
                    else if (type == "label" || type == "status" || type == "percentage")
                    {
                        LabelConfig lbl;
                        lbl.id = elem.value("id", "");
                        lbl.x = elem.value("x", 0);
                        lbl.y = elem.value("y", 0);
                        lbl.width = elem.value("width", 200);
                        lbl.height = elem.value("height", 24);
                        lbl.text = elem.value("text", "");
                        lbl.fontName = elem.value("fontName", "Segoe UI");
                        lbl.fontSize = elem.value("fontSize", 12);
                        lbl.fontColor = elem.value("fontColor", "#ffffff");
                        lbl.fontBold = elem.value("fontBold", false);
                        lbl.fontItalic = elem.value("fontItalic", false);
                        lbl.isStatusLabel = (type == "status");
                        lbl.isPercentageLabel = (type == "percentage");

                        // Efeitos - lê do objeto effects se existir
                        if (elem.contains("effects"))
                        {
                            auto &effects = elem["effects"];
                            lbl.effects.opacity = effects.value("opacity", 100);

                            if (effects.contains("shadow"))
                            {
                                lbl.effects.shadow.enabled = effects["shadow"].value("enabled", false);
                                lbl.effects.shadow.color = effects["shadow"].value("color", "#000000");
                                lbl.effects.shadow.blur = effects["shadow"].value("blur", 2);
                                lbl.effects.shadow.offsetX = effects["shadow"].value("offsetX", 1);
                                lbl.effects.shadow.offsetY = effects["shadow"].value("offsetY", 1);
                            }
                        }

                        config.imageMode->labels.push_back(lbl);
                    }
                    else if (type == "box")
                    {
                        BoxConfig box;
                        box.id = elem.value("id", "");
                        box.x = elem.value("x", 0);
                        box.y = elem.value("y", 0);
                        box.width = elem.value("width", 200);
                        box.height = elem.value("height", 100);

                        // Estilo da caixa
                        if (elem.contains("boxStyle"))
                        {
                            auto &style = elem["boxStyle"];
                            box.style.fillColor = style.value("fillColor", "#000000");
                            box.style.fillOpacity = style.value("fillOpacity", 50);
                            box.style.borderColor = style.value("borderColor", "#ffffff");
                            box.style.borderWidth = style.value("borderWidth", 1);
                            box.style.borderRadius = style.value("borderRadius", 8);
                        }

                        // Efeitos
                        if (elem.contains("effects"))
                        {
                            auto &effects = elem["effects"];
                            box.effects.opacity = effects.value("opacity", 100);

                            if (effects.contains("shadow"))
                            {
                                box.effects.shadow.enabled = effects["shadow"].value("enabled", false);
                                box.effects.shadow.color = effects["shadow"].value("color", "#000000");
                                box.effects.shadow.blur = effects["shadow"].value("blur", 4);
                                box.effects.shadow.offsetX = effects["shadow"].value("offsetX", 2);
                                box.effects.shadow.offsetY = effects["shadow"].value("offsetY", 2);
                            }

                            if (effects.contains("glow"))
                            {
                                box.effects.glow.enabled = effects["glow"].value("enabled", false);
                                box.effects.glow.color = effects["glow"].value("color", "#0078d4");
                                box.effects.glow.intensity = effects["glow"].value("intensity", 50);
                            }
                        }

                        config.imageMode->boxes.push_back(box);
                    }
                    else if (type == "image")
                    {
                        ImageConfig img;
                        img.id = elem.value("id", "");
                        img.x = elem.value("x", 0);
                        img.y = elem.value("y", 0);
                        img.width = elem.value("width", 100);
                        img.height = elem.value("height", 100);
                        img.imagePath = elem.value("backgroundImage", "");

                        // Efeitos
                        if (elem.contains("effects"))
                        {
                            auto &effects = elem["effects"];
                            img.effects.opacity = effects.value("opacity", 100);
                            img.effects.borderRadius = effects.value("borderRadius", 0);

                            if (effects.contains("shadow"))
                            {
                                img.effects.shadow.enabled = effects["shadow"].value("enabled", false);
                                img.effects.shadow.color = effects["shadow"].value("color", "#000000");
                                img.effects.shadow.blur = effects["shadow"].value("blur", 4);
                                img.effects.shadow.offsetX = effects["shadow"].value("offsetX", 2);
                                img.effects.shadow.offsetY = effects["shadow"].value("offsetY", 2);
                            }

                            if (effects.contains("glow"))
                            {
                                img.effects.glow.enabled = effects["glow"].value("enabled", false);
                                img.effects.glow.color = effects["glow"].value("color", "#0078d4");
                                img.effects.glow.intensity = effects["glow"].value("intensity", 50);
                            }
                        }

                        config.imageMode->images.push_back(img);
                    }
                    else if (type == "webview")
                    {
                        WebViewConfig wv;
                        wv.id = elem.value("id", "");
                        wv.name = elem.value("name", "");
                        wv.x = elem.value("x", 0);
                        wv.y = elem.value("y", 0);
                        wv.width = elem.value("width", 300);
                        wv.height = elem.value("height", 200);
                        wv.visible = elem.value("visible", true);
                        wv.zIndex = elem.value("zIndex", 3);

                        // Configurações específicas do webview
                        if (elem.contains("webviewConfig"))
                        {
                            auto &wvConfig = elem["webviewConfig"];
                            wv.url = wvConfig.value("url", "https://example.com");
                            wv.borderRadius = wvConfig.value("borderRadius", 8);
                            wv.borderColor = wvConfig.value("borderColor", "#333333");
                            wv.borderWidth = wvConfig.value("borderWidth", 1);
                            wv.backgroundColor = wvConfig.value("backgroundColor", "#1e1e1e");
                        }

                        config.imageMode->webviews.push_back(wv);
                    }
                }

                // Progress bar do novo formato
                if (j.contains("progressBar"))
                {
                    auto &pb = j["progressBar"];
                    config.imageMode->progressBar.x = pb.value("x", 50);
                    config.imageMode->progressBar.y = pb.value("y", 550);
                    config.imageMode->progressBar.width = pb.value("width", 700);
                    config.imageMode->progressBar.height = pb.value("height", 20);
                    config.imageMode->progressBar.backgroundColor = pb.value("backgroundColor", "#333333");
                    config.imageMode->progressBar.fillColor = pb.value("fillColor", "#00FF00");
                }

                // Video background do novo formato
                if (j.contains("videoBackground"))
                {
                    auto &vb = j["videoBackground"];
                    config.imageMode->videoBackground.enabled = vb.value("enabled", false);
                    config.imageMode->videoBackground.videoFile = vb.value("videoFile", "");
                    config.imageMode->videoBackground.loop = vb.value("loop", true);
                    config.imageMode->videoBackground.autoplay = vb.value("autoplay", true);
                    config.imageMode->videoBackground.muted = vb.value("muted", true);
                    config.imageMode->videoBackground.showControls = vb.value("showControls", false);

                    // Control button style
                    if (vb.contains("controlButton"))
                    {
                        auto &cb = vb["controlButton"];
                        config.imageMode->videoBackground.controlButton.x = cb.value("x", 740);
                        config.imageMode->videoBackground.controlButton.y = cb.value("y", 550);
                        config.imageMode->videoBackground.controlButton.size = cb.value("size", 50);
                        config.imageMode->videoBackground.controlButton.backgroundColor = cb.value("backgroundColor", "#000000");
                        config.imageMode->videoBackground.controlButton.iconColor = cb.value("iconColor", "#ffffff");
                        config.imageMode->videoBackground.controlButton.borderColor = cb.value("borderColor", "#ffffff");
                        config.imageMode->videoBackground.controlButton.borderWidth = cb.value("borderWidth", 2);
                        config.imageMode->videoBackground.controlButton.opacity = cb.value("opacity", 50);
                    }
                }
            }
            // FORMATO ANTIGO: imageMode object
            else if (j.contains("imageMode") && !j["imageMode"].is_null())
            {
                config.imageMode = std::make_shared<ImageModeConfig>();
                auto &im = j["imageMode"];

                config.imageMode->backgroundImage = im.value("backgroundImage", "");

                // Buttons
                if (im.contains("buttons") && im["buttons"].is_array())
                {
                    for (const auto &btn : im["buttons"])
                    {
                        ButtonConfig bc;
                        bc.id = btn.value("id", "");
                        bc.action = btn.value("action", "");
                        bc.x = btn.value("x", 0);
                        bc.y = btn.value("y", 0);
                        bc.width = btn.value("width", 100);
                        bc.height = btn.value("height", 30);
                        bc.text = btn.value("text", "");
                        bc.normalImage = btn.value("normalImage", "");
                        bc.hoverImage = btn.value("hoverImage", "");
                        bc.pressedImage = btn.value("pressedImage", "");
                        config.imageMode->buttons.push_back(bc);
                    }
                }

                // Labels
                if (im.contains("labels") && im["labels"].is_array())
                {
                    for (const auto &lbl : im["labels"])
                    {
                        LabelConfig lc;
                        lc.id = lbl.value("id", "");
                        lc.x = lbl.value("x", 0);
                        lc.y = lbl.value("y", 0);
                        lc.width = lbl.value("width", 200);
                        lc.height = lbl.value("height", 20);
                        lc.text = lbl.value("text", "");
                        lc.fontName = lbl.value("fontFamily", "Segoe UI");
                        lc.fontSize = lbl.value("fontSize", 12);
                        lc.fontColor = lbl.value("fontColor", "#FFFFFF");
                        lc.textAlign = static_cast<TextAlignment>(lbl.value("alignment", 0));
                        config.imageMode->labels.push_back(lc);
                    }
                }

                // Progress bar
                if (im.contains("progressBar"))
                {
                    auto &pb = im["progressBar"];
                    config.imageMode->progressBar.x = pb.value("x", 50);
                    config.imageMode->progressBar.y = pb.value("y", 550);
                    config.imageMode->progressBar.width = pb.value("width", 700);
                    config.imageMode->progressBar.height = pb.value("height", 20);
                    config.imageMode->progressBar.backgroundColor = pb.value("backgroundColor", "#333333");
                    config.imageMode->progressBar.fillColor = pb.value("fillColor", "#007ACC");
                }
            }
            else
            {
                config.imageMode = std::make_shared<ImageModeConfig>();
            }

            // HTML mode config
            if (j.contains("htmlMode") && !j["htmlMode"].is_null())
            {
                config.htmlMode = std::make_shared<HtmlModeConfig>();
                // HTML mode é carregado separadamente via recursos ou strings
            }
        }
        catch (const std::exception &e)
        {
            // Em caso de erro, usar configuração padrão
            config.serverName = "Servidor (erro ao carregar config)";
            config.imageMode = std::make_shared<ImageModeConfig>();
        }

        return config;
    }

} // namespace autopatch
