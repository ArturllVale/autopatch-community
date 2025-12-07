#include "window.h"
#include "../core/resources.h"
#include "../core/config.h"
#include "../core/utils.h"
#include <dwmapi.h>
#include <windowsx.h>
#include <shellapi.h>
#include <fstream>
#include <nlohmann/json.hpp>

#pragma comment(lib, "dwmapi.lib")

namespace autopatch
{
    using utils::StringToWide;
    using json = nlohmann::json;

    // Carrega configuração do recurso embutido
    static std::optional<PatcherConfig> LoadConfigFromResource()
    {
        auto data = Resources::LoadRCData(1001); // ID do recurso CONFIG (1001)
        if (data.empty())
            return std::nullopt;

        try
        {
            std::string jsonStr(data.begin(), data.end());
            auto j = json::parse(jsonStr);

            PatcherConfig config;
            config.serverName = j.value("serverName", "");
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

            if (j.contains("grfFiles"))
            {
                for (const auto &grf : j["grfFiles"])
                {
                    config.grfFiles.push_back(grf.get<std::string>());
                }
            }

            // Novo formato do Vue Builder - elements array + progressBar
            if (j.contains("elements"))
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
                        box.height = elem.value("height", 150);

                        // Estilo do box
                        if (elem.contains("boxStyle"))
                        {
                            auto &style = elem["boxStyle"];
                            box.style.fillColor = style.value("fillColor", "#000000");
                            box.style.fillOpacity = style.value("fillOpacity", 50);
                            box.style.borderColor = style.value("borderColor", "#ffffff");
                            box.style.borderWidth = style.value("borderWidth", 1);
                            box.style.borderRadius = style.value("borderRadius", 8);
                        }
                        else
                        {
                            box.style.fillColor = elem.value("backgroundColor", "#000000");
                            box.style.borderColor = elem.value("borderColor", "#ffffff");
                            box.style.borderWidth = elem.value("borderWidth", 0);
                            box.style.borderRadius = elem.value("borderRadius", 0);
                        }

                        // Efeitos - lê do objeto effects se existir
                        if (elem.contains("effects"))
                        {
                            auto &effects = elem["effects"];
                            box.effects.opacity = effects.value("opacity", 100);
                            box.effects.borderRadius = effects.value("borderRadius", box.style.borderRadius);

                            if (effects.contains("shadow"))
                            {
                                box.effects.shadow.enabled = effects["shadow"].value("enabled", false);
                                box.effects.shadow.color = effects["shadow"].value("color", "#000000");
                                box.effects.shadow.blur = effects["shadow"].value("blur", 10);
                                box.effects.shadow.offsetX = effects["shadow"].value("offsetX", 0);
                                box.effects.shadow.offsetY = effects["shadow"].value("offsetY", 4);
                            }

                            if (effects.contains("glow"))
                            {
                                box.effects.glow.enabled = effects["glow"].value("enabled", false);
                                box.effects.glow.color = effects["glow"].value("color", "#00ff00");
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

                        // Efeitos - lê do objeto effects se existir
                        if (elem.contains("effects"))
                        {
                            auto &effects = elem["effects"];
                            img.effects.opacity = effects.value("opacity", 100);
                            img.effects.borderRadius = effects.value("borderRadius", 0);

                            if (effects.contains("shadow"))
                            {
                                img.effects.shadow.enabled = effects["shadow"].value("enabled", false);
                                img.effects.shadow.color = effects["shadow"].value("color", "#000000");
                                img.effects.shadow.blur = effects["shadow"].value("blur", 10);
                                img.effects.shadow.offsetX = effects["shadow"].value("offsetX", 0);
                                img.effects.shadow.offsetY = effects["shadow"].value("offsetY", 4);
                            }

                            if (effects.contains("glow"))
                            {
                                img.effects.glow.enabled = effects["glow"].value("enabled", false);
                                img.effects.glow.color = effects["glow"].value("color", "#00ff00");
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
                    auto &pbJ = j["progressBar"];
                    config.imageMode->progressBar.x = pbJ.value("x", 0);
                    config.imageMode->progressBar.y = pbJ.value("y", 0);
                    config.imageMode->progressBar.width = pbJ.value("width", 400);
                    config.imageMode->progressBar.height = pbJ.value("height", 20);
                    config.imageMode->progressBar.backgroundColor = pbJ.value("backgroundColor", "#333333");
                    config.imageMode->progressBar.fillColor = pbJ.value("fillColor", "#00FF00");
                    config.imageMode->progressBar.borderRadius = pbJ.value("borderRadius", 0);
                }
            }
            // Formato antigo - imageMode object
            else if (j.contains("imageMode"))
            {
                config.imageMode = std::make_shared<ImageModeConfig>();
                auto &im = j["imageMode"];
                config.imageMode->backgroundImage = im.value("backgroundImage", "");

                if (im.contains("buttons"))
                {
                    for (const auto &btnJ : im["buttons"])
                    {
                        ButtonConfig btn;
                        btn.id = btnJ.value("id", "");
                        btn.action = btnJ.value("action", "");
                        btn.x = btnJ.value("x", 0);
                        btn.y = btnJ.value("y", 0);
                        btn.width = btnJ.value("width", 100);
                        btn.height = btnJ.value("height", 30);
                        btn.text = btnJ.value("text", "");
                        btn.normalImage = btnJ.value("normalImage", "");
                        btn.hoverImage = btnJ.value("hoverImage", "");
                        btn.pressedImage = btnJ.value("pressedImage", "");
                        config.imageMode->buttons.push_back(btn);
                    }
                }

                if (im.contains("labels"))
                {
                    for (const auto &lblJ : im["labels"])
                    {
                        LabelConfig lbl;
                        lbl.id = lblJ.value("id", "");
                        lbl.x = lblJ.value("x", 0);
                        lbl.y = lblJ.value("y", 0);
                        lbl.width = lblJ.value("width", 200);
                        lbl.height = lblJ.value("height", 24);
                        lbl.text = lblJ.value("text", "");
                        lbl.fontName = lblJ.value("fontName", "Segoe UI");
                        lbl.fontSize = lblJ.value("fontSize", 12);
                        lbl.fontColor = lblJ.value("fontColor", "#ffffff");
                        config.imageMode->labels.push_back(lbl);
                    }
                }

                if (im.contains("progressBar"))
                {
                    auto &pbJ = im["progressBar"];
                    config.imageMode->progressBar.x = pbJ.value("x", 0);
                    config.imageMode->progressBar.y = pbJ.value("y", 0);
                    config.imageMode->progressBar.width = pbJ.value("width", 400);
                    config.imageMode->progressBar.height = pbJ.value("height", 20);
                    config.imageMode->progressBar.backgroundColor = pbJ.value("backgroundColor", "#333333");
                    config.imageMode->progressBar.fillColor = pbJ.value("fillColor", "#00FF00");
                }
            }

            return config;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    // Carrega configuração de arquivo JSON
    static std::optional<PatcherConfig> LoadConfigFromFile(const std::wstring &path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            return std::nullopt;

        try
        {
            json j;
            file >> j;
            file.close();

            PatcherConfig config;
            config.serverName = j.value("serverName", "");
            config.patchListUrl = j.value("patchListUrl", "");
            config.newsUrl = j.value("newsUrl", "");
            config.clientExe = j.value("clientExe", "ragexe.exe");
            config.clientArgs = j.value("clientArgs", "");
            config.windowWidth = j.value("windowWidth", 800);
            config.windowHeight = j.value("windowHeight", 600);
            config.uiType = static_cast<UIType>(j.value("uiType", 0));

            if (j.contains("grfFiles"))
            {
                for (const auto &grf : j["grfFiles"])
                {
                    config.grfFiles.push_back(grf.get<std::string>());
                }
            }

            if (j.contains("imageMode"))
            {
                config.imageMode = std::make_shared<ImageModeConfig>();
                auto &im = j["imageMode"];
                config.imageMode->backgroundImage = im.value("backgroundImage", "");

                if (im.contains("buttons"))
                {
                    for (const auto &btnJ : im["buttons"])
                    {
                        ButtonConfig btn;
                        btn.id = btnJ.value("id", "");
                        btn.action = btnJ.value("action", "");
                        btn.x = btnJ.value("x", 0);
                        btn.y = btnJ.value("y", 0);
                        btn.width = btnJ.value("width", 100);
                        btn.height = btnJ.value("height", 30);
                        btn.text = btnJ.value("text", "");
                        btn.normalImage = btnJ.value("normalImage", "");
                        btn.hoverImage = btnJ.value("hoverImage", "");
                        btn.pressedImage = btnJ.value("pressedImage", "");
                        config.imageMode->buttons.push_back(btn);
                    }
                }

                if (im.contains("progressBar"))
                {
                    auto &pbJ = im["progressBar"];
                    config.imageMode->progressBar.x = pbJ.value("x", 0);
                    config.imageMode->progressBar.y = pbJ.value("y", 0);
                    config.imageMode->progressBar.width = pbJ.value("width", 400);
                    config.imageMode->progressBar.height = pbJ.value("height", 20);
                    config.imageMode->progressBar.backgroundColor = pbJ.value("backgroundColor", "#333333");
                    config.imageMode->progressBar.fillColor = pbJ.value("fillColor", "#00FF00");
                }
            }

            return config;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    MainWindow *MainWindow::s_instance = nullptr;

    MainWindow::MainWindow()
    {
        s_instance = this;
        m_ui = std::make_unique<UI>();
    }

    MainWindow::~MainWindow()
    {
        s_instance = nullptr;
    }

    bool MainWindow::Create(HINSTANCE hInstance)
    {
        m_hInstance = hInstance;

        // Inicializa GDI+
        if (!UI::InitializeGdiPlus())
        {
            MessageBoxW(nullptr, L"Falha ao inicializar GDI+", L"Erro", MB_ICONERROR);
            return false;
        }

        // Carrega configuração embutida ou de arquivo
        m_config = LoadConfig();

        // Verifica se configuração é válida
        if (m_config.patchListUrl.empty() && m_config.serverName.find("erro") != std::string::npos)
        {
            MessageBoxW(nullptr, L"Configuração não encontrada", L"Erro", MB_ICONERROR);
            return false;
        }

        // Carrega skin
        if (!m_ui->LoadSkin(m_config))
        {
            MessageBoxW(nullptr, L"Falha ao carregar skin", L"Erro", MB_ICONERROR);
            return false;
        }

        // Configura callback de ações
        m_ui->SetActionCallback([this](const std::wstring &action)
                                { OnButtonAction(action); });

        // Registra classe da janela
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"AutoPatcherWindow";
        wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
        wc.hIconSm = wc.hIcon;

        if (!RegisterClassExW(&wc))
        {
            return false;
        }

        // Calcula tamanho da janela
        int width = m_ui->GetWindowWidth();
        int height = m_ui->GetWindowHeight();

        // Centraliza na tela
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int x = (screenWidth - width) / 2;
        int y = (screenHeight - height) / 2;

        // Cria janela sem borda (popup)
        m_hwnd = CreateWindowExW(
            WS_EX_LAYERED,
            L"AutoPatcherWindow",
            StringToWide(m_config.serverName).c_str(),
            WS_POPUP,
            x, y, width, height,
            nullptr, nullptr, hInstance, this);

        if (!m_hwnd)
        {
            return false;
        }

        // Aplica bordas arredondadas se configurado
        if (m_config.windowBorderRadius > 0)
        {
            HRGN hRgn = CreateRoundRectRgn(0, 0, width + 1, height + 1,
                                           m_config.windowBorderRadius,
                                           m_config.windowBorderRadius);
            SetWindowRgn(m_hwnd, hRgn, TRUE);
            // Nota: O sistema assume propriedade da região, não deletar
        }

        // Configura transparência
        SetLayeredWindowAttributes(m_hwnd, 0, 255, LWA_ALPHA);

        // Habilita sombra (Windows Vista+)
        MARGINS margins = {-1};
        DwmExtendFrameIntoClientArea(m_hwnd, &margins);

        // Cria WebViews (após a janela estar criada)
        if (m_config.imageMode)
        {
            for (const auto &wv : m_config.imageMode->webviews)
            {
                m_ui->AddWebView(wv, m_hwnd);
            }

            // Inicializa vídeo de fundo se configurado
            const auto &vb = m_config.imageMode->videoBackground;
            if (vb.enabled && !vb.videoFile.empty())
            {
                // Obtém o diretório do EXE
                wchar_t exePath[MAX_PATH];
                GetModuleFileNameW(nullptr, exePath, MAX_PATH);
                std::wstring exeDir(exePath);
                size_t lastSlash = exeDir.find_last_of(L"\\/");
                if (lastSlash != std::wstring::npos)
                {
                    exeDir = exeDir.substr(0, lastSlash + 1);
                }

                // Converte nome do arquivo para wide string
                std::wstring videoFileName;
                int size = MultiByteToWideChar(CP_UTF8, 0, vb.videoFile.c_str(), -1, nullptr, 0);
                if (size > 0)
                {
                    videoFileName.resize(size - 1);
                    MultiByteToWideChar(CP_UTF8, 0, vb.videoFile.c_str(), -1, &videoFileName[0], size);
                }

                // Caminho completo do vídeo (na subpasta resources/)
                std::wstring videoPath = exeDir + L"resources\\" + videoFileName;

                // Pega estilo do botão de controle
                const auto &btn = vb.controlButton;

                if (m_ui->InitializeVideo(m_hwnd, videoPath, vb.loop, vb.autoplay, vb.muted, vb.showControls,
                                          btn.x, btn.y, btn.size, btn.backgroundColor, btn.iconColor,
                                          btn.borderColor, btn.borderWidth, btn.opacity))
                {
                    // Vídeo inicializado com sucesso
                    // Os controles serão mostrados se showControls estiver habilitado
                }
            }
        }

        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);

        return true;
    }

    int MainWindow::Run()
    {
        MSG msg = {};

        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        UI::ShutdownGdiPlus();

        return static_cast<int>(msg.wParam);
    }

    LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow *window = nullptr;

        if (msg == WM_NCCREATE)
        {
            auto cs = reinterpret_cast<CREATESTRUCT *>(lParam);
            window = static_cast<MainWindow *>(cs->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
            window->m_hwnd = hwnd;
        }
        else
        {
            window = reinterpret_cast<MainWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (window)
        {
            return window->HandleMessage(msg, wParam, lParam);
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    LRESULT MainWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_CREATE:
            OnCreate();
            return 0;

        case WM_DESTROY:
            OnDestroy();
            return 0;

        case WM_PAINT:
            OnPaint();
            return 0;

        case WM_MOUSEMOVE:
            OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_LBUTTONDOWN:
            OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_LBUTTONUP:
            OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_TIMER:
            OnTimer(wParam);
            return 0;

        case WM_NCHITTEST:
        {
            // Permite arrastar a janela
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(m_hwnd, &pt);

            if (m_ui->IsInDragRegion(pt.x, pt.y))
            {
                return HTCAPTION;
            }
            return HTCLIENT;
        }

        case WM_ERASEBKGND:
            return 1; // Handled

        case WM_USER + 100: // Mensagem de progresso do patcher
        {
            PatcherStatus status = static_cast<PatcherStatus>(wParam);
            auto *data = reinterpret_cast<std::pair<std::wstring, float> *>(lParam);
            if (data)
            {
                std::wstring message = data->first;
                float progress = data->second;
                delete data;

                switch (status)
                {
                case PatcherStatus::CheckingUpdates:
                    SetStatus(L"Verificando atualizações...");
                    SetProgress(progress);
                    break;

                case PatcherStatus::Downloading:
                    SetStatus(message);
                    SetProgress(progress);
                    break;

                case PatcherStatus::Patching:
                    SetStatus(message);
                    SetProgress(progress);
                    break;

                case PatcherStatus::Complete:
                    SetStatus(L"Atualização concluída!");
                    SetProgress(1.0f);
                    m_ui->EnableButton(L"start_game", true);
                    m_ui->EnableButton(L"check_files", true);
                    break;

                case PatcherStatus::Error:
                {
                    SetStatus(L"Erro: " + message);
                    SetProgress(0.0f);
                    m_ui->EnableButton(L"start_game", true); // Permite jogar mesmo com erro
                    m_ui->EnableButton(L"check_files", true);

                    // Mostra detalhes do erro
                    std::wstring errorMsg = L"Erro ao verificar atualizações:\n\n" + message;
                    errorMsg += L"\n\nURL: " + StringToWide(m_config.patchListUrl);
                    errorMsg += L"\n\nVocê pode continuar jogando, mas pode haver atualizações pendentes.";
                    MessageBoxW(m_hwnd, errorMsg.c_str(), L"Aviso", MB_ICONWARNING);
                    break;
                }

                default:
                    break;
                }
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return 0;
        }
        }

        return DefWindowProc(m_hwnd, msg, wParam, lParam);
    }

    void MainWindow::OnCreate()
    {
        // Os labels de status (ID=1) e porcentagem (ID=2) são criados pelo LoadSkin()
        // a partir da configuração. Não criamos fallbacks - se o usuário não definiu,
        // simplesmente não aparecem.

        // Desabilita botão de iniciar até verificar patches
        m_ui->EnableButton(L"start_game", false);

        // Inicia timer para atualização da UI
        SetTimer(m_hwnd, TIMER_UPDATE, 100, nullptr);

        // Inicia verificação de patches em background
        StartPatchCheck();
    }

    void MainWindow::OnDestroy()
    {
        KillTimer(m_hwnd, TIMER_UPDATE);
        PostQuitMessage(0);
    }

    void MainWindow::OnPaint()
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);

        // Double buffering
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memDC, memBitmap));

        // Renderiza UI (já exclui regiões de WebViews via clipping)
        m_ui->Render(memDC, rc.right, rc.bottom);

        // Cria região de clipping no HDC de destino para excluir áreas dos WebViews
        // Isso evita que o BitBlt sobrescreva os controles WebBrowser
        auto webViewRects = m_ui->GetWebViewRects();
        HRGN clipRegion = CreateRectRgn(0, 0, rc.right, rc.bottom);
        for (const auto &wvRect : webViewRects)
        {
            HRGN wvRegion = CreateRectRgn(wvRect.left, wvRect.top, wvRect.right, wvRect.bottom);
            CombineRgn(clipRegion, clipRegion, wvRegion, RGN_DIFF);
            DeleteObject(wvRegion);
        }
        SelectClipRgn(hdc, clipRegion);

        // Copia para tela (agora respeitando o clipping que exclui WebViews)
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

        // Remove clipping e limpa região
        SelectClipRgn(hdc, NULL);
        DeleteObject(clipRegion);

        // Cleanup
        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);

        EndPaint(m_hwnd, &ps);

        // Garante que os WebViews permaneçam visíveis após o paint
        m_ui->BringWebViewsToFront();
    }

    void MainWindow::OnMouseMove(int x, int y)
    {
        if (m_dragging)
        {
            POINT pt;
            GetCursorPos(&pt);

            RECT rc;
            GetWindowRect(m_hwnd, &rc);

            SetWindowPos(m_hwnd, nullptr,
                         rc.left + pt.x - m_dragStart.x,
                         rc.top + pt.y - m_dragStart.y,
                         0, 0, SWP_NOSIZE | SWP_NOZORDER);

            m_dragStart = pt;
        }

        if (m_ui->OnMouseMove(x, y))
        {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }

    void MainWindow::OnLButtonDown(int x, int y)
    {
        SetCapture(m_hwnd);

        if (m_ui->IsInDragRegion(x, y))
        {
            m_dragging = true;
            GetCursorPos(&m_dragStart);
        }
        else
        {
            if (m_ui->OnMouseDown(x, y))
            {
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
        }
    }

    void MainWindow::OnLButtonUp(int x, int y)
    {
        ReleaseCapture();
        m_dragging = false;

        if (m_ui->OnMouseUp(x, y))
        {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }

    void MainWindow::OnTimer(UINT_PTR timerId)
    {
        if (timerId == TIMER_UPDATE)
        {
            // Atualiza UI se patcher está rodando
            if (m_patcher)
            {
                // TODO: Verificar progresso do patcher
            }
        }
    }

    void MainWindow::OnButtonAction(const std::wstring &action)
    {
        if (action == L"start_game")
        {
            StartGame();
        }
        else if (action == L"check_files")
        {
            CheckFiles();
        }
        else if (action == L"settings")
        {
            OpenSettings();
        }
        else if (action == L"minimize")
        {
            MinimizeWindow();
        }
        else if (action == L"close" || action == L"exit")
        {
            CloseWindow();
        }
        else if (action == L"toggle_video")
        {
            // Toggle video play/pause
            if (m_ui->HasVideo())
            {
                m_ui->ToggleVideo();
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
        }
        else if (action == L"play_video")
        {
            if (m_ui->HasVideo())
            {
                m_ui->PlayVideo();
            }
        }
        else if (action == L"pause_video")
        {
            if (m_ui->HasVideo())
            {
                m_ui->PauseVideo();
            }
        }
    }

    void MainWindow::StartGame()
    {
        // Executa o jogo
        STARTUPINFOW si = {};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi = {};

        std::wstring exePath = StringToWide(m_config.clientExe);
        std::wstring cmdLine = exePath;
        if (!m_config.clientArgs.empty())
        {
            cmdLine += L" " + StringToWide(m_config.clientArgs);
        }

        // Obtém o diretório atual
        wchar_t currentDir[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, currentDir);

        // Tenta CreateProcessW primeiro
        if (CreateProcessW(exePath.c_str(), cmdLine.data(), nullptr, nullptr, FALSE,
                           0, nullptr, currentDir, &si, &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            // Fecha o patcher após iniciar o jogo
            CloseWindow();
        }
        else
        {
            // Tenta ShellExecuteW como fallback
            HINSTANCE result = ShellExecuteW(m_hwnd, L"open", exePath.c_str(),
                                             m_config.clientArgs.empty() ? nullptr : StringToWide(m_config.clientArgs).c_str(),
                                             currentDir, SW_SHOW);

            if ((INT_PTR)result > 32)
            {
                // Sucesso com ShellExecute
                CloseWindow();
            }
            else
            {
                // Mostra erro detalhado
                DWORD error = GetLastError();
                wchar_t msg[512];
                swprintf_s(msg, L"Falha ao iniciar o jogo.\n\nExecutável: %s\nDiretório: %s\nErro: %lu",
                           exePath.c_str(), currentDir, error);
                MessageBoxW(m_hwnd, msg, L"Erro", MB_ICONERROR);
            }
        }
    }

    void MainWindow::CheckFiles()
    {
        SetStatus(L"Verificando arquivos...");
        m_ui->EnableButton(L"start_game", false);
        m_ui->EnableButton(L"check_files", false);
        InvalidateRect(m_hwnd, nullptr, FALSE);

        StartPatchCheck();
    }

    void MainWindow::OpenSettings()
    {
        // TODO: Implementar janela de configurações
        MessageBoxW(m_hwnd, L"Configurações em desenvolvimento", L"Info", MB_ICONINFORMATION);
    }

    void MainWindow::MinimizeWindow()
    {
        ShowWindow(m_hwnd, SW_MINIMIZE);
    }

    void MainWindow::CloseWindow()
    {
        DestroyWindow(m_hwnd);
    }

    void MainWindow::SetStatus(const std::wstring &text)
    {
        m_ui->SetLabelText(1, text);
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    void MainWindow::SetProgress(float progress)
    {
        m_ui->SetProgress(progress);

        int percent = static_cast<int>(progress * 100);
        m_ui->SetLabelText(2, std::to_wstring(percent) + L"%");

        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    void MainWindow::StartPatchCheck()
    {
        // Verifica se tem URL de patchlist configurada
        if (m_config.patchListUrl.empty())
        {
            // Sem URL configurada - habilita o botão direto
            SetStatus(L"Pronto para jogar!");
            SetProgress(1.0f);
            m_ui->EnableButton(L"start_game", true);
            m_ui->EnableButton(L"check_files", true);
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return;
        }

        // Inicializa o patcher se necessário
        if (!m_patcher)
        {
            m_patcher = std::make_unique<Patcher>();
            m_patcher->Initialize(m_config);
        }

        // Define callback de progresso
        m_patcher->SetProgressCallback([this](PatcherStatus status,
                                              const std::wstring &message,
                                              float progress)
                                       {
            // Executa na thread principal
            PostMessage(m_hwnd, WM_USER + 100, static_cast<WPARAM>(status), 
                       reinterpret_cast<LPARAM>(new std::pair<std::wstring, float>(message, progress))); });

        // Inicia verificação
        SetStatus(L"Verificando atualizações...");
        SetProgress(0.0f);
        m_patcher->CheckForUpdates();
    }

} // namespace autopatch
