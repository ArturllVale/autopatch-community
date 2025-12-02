#include "builder_window.h"
#include "../core/utils.h"
#include <CommCtrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <windowsx.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Comdlg32.lib")

namespace autopatch
{
    using json = nlohmann::json;

    ModernBuilderWindow::ModernBuilderWindow()
    {
        // Valores padrão do projeto
        m_project.serverName = L"Meu Servidor RO";
        m_project.baseUrl = L"https://seuservidor.com/patch/";
        m_project.patchlistFile = L"patchlist.txt";
        m_project.patchesFolder = L"patches/";
        m_project.mainGrf = L"data.grf";
        m_project.gameExecutable = L"ragexe.exe";
        m_project.windowWidth = 800;
        m_project.windowHeight = 600;
    }

    ModernBuilderWindow::~ModernBuilderWindow()
    {
        ShutdownGdiPlus();
    }

    void ModernBuilderWindow::InitGdiPlus()
    {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);
    }

    void ModernBuilderWindow::ShutdownGdiPlus()
    {
        if (m_gdiplusToken)
        {
            Gdiplus::GdiplusShutdown(m_gdiplusToken);
            m_gdiplusToken = 0;
        }
    }

    bool ModernBuilderWindow::Create(HINSTANCE hInstance)
    {
        m_hInstance = hInstance;
        InitGdiPlus();

        // Inicializa Common Controls
        INITCOMMONCONTROLSEX icex = {};
        icex.dwSize = sizeof(icex);
        icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
        InitCommonControlsEx(&icex);

        // Registra classe
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_DBLCLKS; // Sem CS_HREDRAW | CS_VREDRAW para evitar flicker
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr; // Vamos desenhar tudo
        wc.lpszClassName = L"ModernBuilderWindow";
        wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

        if (!RegisterClassExW(&wc))
            return false;

        // Centraliza
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int x = (screenW - m_width) / 2;
        int y = (screenH - m_height) / 2;

        m_hwnd = CreateWindowExW(
            WS_EX_COMPOSITED, // Double buffering
            L"ModernBuilderWindow",
            L"AutoPatch Builder",
            WS_OVERLAPPEDWINDOW,
            x, y, m_width, m_height,
            nullptr, nullptr, hInstance, this);

        if (!m_hwnd)
            return false;

        CreateUI();
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);

        return true;
    }

    int ModernBuilderWindow::Run()
    {
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return (int)msg.wParam;
    }

    LRESULT CALLBACK ModernBuilderWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        ModernBuilderWindow *pThis = nullptr;

        if (msg == WM_NCCREATE)
        {
            CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
            pThis = (ModernBuilderWindow *)cs->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            pThis->m_hwnd = hwnd;
        }
        else
        {
            pThis = (ModernBuilderWindow *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        }

        if (pThis)
            return pThis->HandleMessage(msg, wParam, lParam);

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT ModernBuilderWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_PAINT:
            OnPaint();
            return 0;

        case WM_ERASEBKGND:
            return 1; // Não apaga fundo

        case WM_SIZE:
            OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_MOUSEMOVE:
            OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            if (!m_isTracking)
            {
                m_trackMouse.cbSize = sizeof(m_trackMouse);
                m_trackMouse.dwFlags = TME_LEAVE;
                m_trackMouse.hwndTrack = m_hwnd;
                TrackMouseEvent(&m_trackMouse);
                m_isTracking = true;
            }
            return 0;

        case WM_MOUSELEAVE:
            m_isTracking = false;
            Invalidate();
            return 0;

        case WM_LBUTTONDOWN:
            OnMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            SetCapture(m_hwnd);
            return 0;

        case WM_LBUTTONUP:
            OnMouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            ReleaseCapture();
            return 0;

        case WM_COMMAND:
            OnCommand(wParam, lParam);
            return 0;

        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, Colors::Text);
            SetBkColor(hdc, Colors::Tertiary);
            return (LRESULT)Drawing::CreateSolidBrushCached(Colors::Tertiary);
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }

    void ModernBuilderWindow::CreateUI()
    {
        CreateSidebar();
        CreateMainArea();
        CreateFooter();
        CreateEditControls();
    }

    void ModernBuilderWindow::CreateSidebar()
    {
        // Mode cards
        m_imageModeCard = std::make_unique<ModeCard>();
        m_imageModeCard->title = L"Modo Imagem";
        m_imageModeCard->description = L"Background + botoes";
        m_imageModeCard->icon = L"[IMG]";
        m_imageModeCard->isSelected = true;
        m_imageModeCard->onClick = [this]()
        { SelectImageMode(); };

        m_htmlModeCard = std::make_unique<ModeCard>();
        m_htmlModeCard->title = L"Modo HTML/CSS/JS";
        m_htmlModeCard->description = L"Interface web customizada";
        m_htmlModeCard->icon = L"[WEB]";
        m_htmlModeCard->onClick = [this]()
        { SelectHtmlMode(); };

        // Checkbox
        m_chkCloseAfterStart = std::make_unique<ModernCheckBox>();
        m_chkCloseAfterStart->text = L"Fechar ao iniciar jogo";
        m_chkCloseAfterStart->isChecked = true;
        m_chkCloseAfterStart->onChanged = [this](bool checked)
        {
            m_project.closeAfterStart = checked;
        };
    }

    void ModernBuilderWindow::CreateMainArea()
    {
        // Canvas
        m_canvas = std::make_unique<DesignCanvas>();
        m_canvas->project = &m_project;
        m_canvas->onSelectionChanged = [this](UIElementData *elem)
        {
            UpdatePropertiesPanel();
            Invalidate();
        };

        // Botoes de adicionar elementos
        m_btnSelectBg = std::make_unique<ModernButton>();
        m_btnSelectBg->text = L"Selecionar Background";
        m_btnSelectBg->onClick = [this]()
        { SelectBackgroundImage(); };

        m_btnAddButton = std::make_unique<ModernButton>();
        m_btnAddButton->text = L"+ Botao";
        m_btnAddButton->onClick = [this]()
        { AddElement(0); };

        m_btnAddLabel = std::make_unique<ModernButton>();
        m_btnAddLabel->text = L"+ Status";
        m_btnAddLabel->onClick = [this]()
        { AddElement(3); }; // Status label

        m_btnAddProgress = std::make_unique<ModernButton>();
        m_btnAddProgress->text = L"+ %";
        m_btnAddProgress->onClick = [this]()
        { AddElement(4); }; // Percentagem label

        m_btnAddProgressBar = std::make_unique<ModernButton>();
        m_btnAddProgressBar->text = L"+ Barra";
        m_btnAddProgressBar->onClick = [this]()
        { AddElement(2); }; // ProgressBar

        m_btnRemoveElement = std::make_unique<ModernButton>();
        m_btnRemoveElement->text = L"Remover";
        m_btnRemoveElement->onClick = [this]()
        { RemoveSelectedElement(); };
    }

    void ModernBuilderWindow::CreateFooter()
    {
        m_btnOpenProject = std::make_unique<ModernButton>();
        m_btnOpenProject->text = L"Abrir Projeto";
        m_btnOpenProject->onClick = [this]()
        { OpenProject(); };

        m_btnSaveProject = std::make_unique<ModernButton>();
        m_btnSaveProject->text = L"Salvar Projeto";
        m_btnSaveProject->onClick = [this]()
        { SaveProject(); };

        m_btnPreview = std::make_unique<ModernButton>();
        m_btnPreview->text = L"Pre-visualizar";
        m_btnPreview->onClick = [this]()
        { Preview(); };

        m_btnGenerate = std::make_unique<ModernButton>();
        m_btnGenerate->text = L"Gerar Patcher EXE";
        m_btnGenerate->isPrimary = true;
        m_btnGenerate->onClick = [this]()
        { ShowExportDialog(); };
    }

    void ModernBuilderWindow::CreateEditControls()
    {
        // Cria controles de edicao nativos
        auto createEdit = [this](int id, int x, int y, int w) -> HWND
        {
            HWND hwnd = CreateWindowExW(0, L"EDIT", L"",
                                        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_BORDER,
                                        x, y, w, 24,
                                        m_hwnd, (HMENU)(INT_PTR)id, m_hInstance, nullptr);

            HFONT font = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            SendMessage(hwnd, WM_SETFONT, (WPARAM)font, TRUE);
            return hwnd;
        };

        int x = 20;
        int w = SIDEBAR_WIDTH - 40;
        int editH = 24;
        int spacing = 42; // Espaçamento entre campos

        // Calcula posições baseado no layout do PaintSidebar
        // header=70, y_start=85, TIPO DE INTERFACE=25, cards=55+60=115
        // Total antes dos campos: 70+15+25+115 = 225
        int y = 225;

        // Nome do Servidor
        m_editServerName = createEdit(ID_EDIT_SERVER_NAME, x, y + 18, w);
        SetWindowTextW(m_editServerName, m_project.serverName.c_str());
        y += spacing;

        // URL Base
        m_editBaseUrl = createEdit(ID_EDIT_BASE_URL, x, y + 18, w);
        SetWindowTextW(m_editBaseUrl, m_project.baseUrl.c_str());
        y += spacing;

        // Patchlist
        m_editPatchlist = createEdit(ID_EDIT_PATCHLIST, x, y + 18, w);
        SetWindowTextW(m_editPatchlist, m_project.patchlistFile.c_str());
        y += spacing;

        // Pasta Patches
        m_editPatchesFolder = createEdit(ID_EDIT_PATCHES_FOLDER, x, y + 18, w);
        SetWindowTextW(m_editPatchesFolder, m_project.patchesFolder.c_str());
        y += spacing + 5; // +5 para seção

        // Seção JOGO (+20 para titulo)
        y += 20;

        // GRF Principal
        m_editMainGrf = createEdit(ID_EDIT_MAIN_GRF, x, y + 18, w);
        SetWindowTextW(m_editMainGrf, m_project.mainGrf.c_str());
        y += spacing;

        // Executavel
        m_editGameExe = createEdit(ID_EDIT_GAME_EXE, x, y + 18, w);
        SetWindowTextW(m_editGameExe, m_project.gameExecutable.c_str());
        y += spacing;

        // Tamanho da Janela
        m_editWindowWidth = createEdit(ID_EDIT_WINDOW_WIDTH, x, y + 18, 60);
        SetWindowTextW(m_editWindowWidth, std::to_wstring(m_project.windowWidth).c_str());

        m_editWindowHeight = createEdit(ID_EDIT_WINDOW_HEIGHT, x + 80, y + 18, 60);
        SetWindowTextW(m_editWindowHeight, std::to_wstring(m_project.windowHeight).c_str());

        // Controles de propriedade do elemento (no painel direito)
        // Esses serao reposicionados em UpdatePropertiesEditPositions
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        int propX = rc.right - PROPERTIES_WIDTH + 16;
        int propW = PROPERTIES_WIDTH - 32;

        // Inicialmente ocultos (serao mostrados quando um elemento for selecionado)
        auto createPropEdit = [this](int id, int x, int y, int w, bool hidden = true) -> HWND
        {
            HWND hwnd = CreateWindowExW(0, L"EDIT", L"",
                                        WS_CHILD | ES_AUTOHSCROLL | WS_BORDER | (hidden ? 0 : WS_VISIBLE),
                                        x, y, w, 22,
                                        m_hwnd, (HMENU)(INT_PTR)id, m_hInstance, nullptr);

            HFONT font = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            SendMessage(hwnd, WM_SETFONT, (WPARAM)font, TRUE);
            return hwnd;
        };

        m_editElemX = createPropEdit(ID_EDIT_ELEM_X, propX, 150, 60);
        m_editElemY = createPropEdit(ID_EDIT_ELEM_Y, propX + 80, 150, 60);
        m_editElemW = createPropEdit(ID_EDIT_ELEM_W, propX, 200, 60);
        m_editElemH = createPropEdit(ID_EDIT_ELEM_H, propX + 80, 200, 60);
        m_editElemText = createPropEdit(ID_EDIT_ELEM_TEXT, propX, 250, propW);

        // ComboBox para acao
        m_comboElemAction = CreateWindowExW(0, L"COMBOBOX", L"",
                                            WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                                            propX, 300, propW, 200,
                                            m_hwnd, (HMENU)(INT_PTR)ID_COMBO_ELEM_ACTION, m_hInstance, nullptr);

        HFONT comboFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        SendMessage(m_comboElemAction, WM_SETFONT, (WPARAM)comboFont, TRUE);

        // Adiciona opcoes de acao
        SendMessageW(m_comboElemAction, CB_ADDSTRING, 0, (LPARAM)L"start_game");
        SendMessageW(m_comboElemAction, CB_ADDSTRING, 0, (LPARAM)L"check_updates");
        SendMessageW(m_comboElemAction, CB_ADDSTRING, 0, (LPARAM)L"exit");
        SendMessageW(m_comboElemAction, CB_ADDSTRING, 0, (LPARAM)L"open_url");
        SendMessageW(m_comboElemAction, CB_ADDSTRING, 0, (LPARAM)L"open_folder");
        SendMessageW(m_comboElemAction, CB_SETCURSEL, 0, 0);
    }

    void ModernBuilderWindow::OnPaint()
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);

        RECT rc;
        GetClientRect(m_hwnd, &rc);

        // Double buffer
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

        // Fundo
        Drawing::FillRect(memDC, rc, Colors::Primary);

        // Header
        RECT header = {0, 0, rc.right, 70};
        Drawing::FillRect(memDC, header, Colors::Secondary);

        RECT titleRect = {20, 18, 400, 45};
        Drawing::DrawTextBold(memDC, L"AutoPatch Builder", titleRect, Colors::White, 20);

        RECT subtitleRect = {20, 45, 500, 65};
        Drawing::DrawText(memDC, L"Crie seu patcher personalizado para Ragnarok Online",
                          subtitleRect, Colors::TextSecondary, 12);

        // Sidebar
        PaintSidebar(memDC);

        // Main area
        PaintMainArea(memDC);

        // Footer
        PaintFooter(memDC);

        // Copia para tela
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);

        EndPaint(m_hwnd, &ps);
    }

    void ModernBuilderWindow::PaintSidebar(HDC hdc)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        // Background sidebar
        RECT sidebar = {0, 70, SIDEBAR_WIDTH, rc.bottom - FOOTER_HEIGHT};
        Drawing::FillRect(hdc, sidebar, Colors::Secondary);

        int y = 85;

        // Secao: Tipo de Interface
        RECT labelRect = {20, y, SIDEBAR_WIDTH - 20, y + 20};
        Drawing::DrawTextBold(hdc, L"TIPO DE INTERFACE", labelRect, Colors::Accent, 11);
        y += 25;

        // Mode Cards (menores)
        m_imageModeCard->bounds = {16, y, SIDEBAR_WIDTH - 16, y + 50};
        m_imageModeCard->Paint(hdc, m_imageModeCard->bounds);
        y += 55;

        m_htmlModeCard->bounds = {16, y, SIDEBAR_WIDTH - 16, y + 50};
        m_htmlModeCard->Paint(hdc, m_htmlModeCard->bounds);
        y += 60;

        int spacing = 42;

        // Nome do Servidor
        labelRect = {20, y, SIDEBAR_WIDTH - 20, y + 18};
        Drawing::DrawText(hdc, L"Nome do Servidor:", labelRect, Colors::TextSecondary, 11);
        y += spacing;

        // URL Base
        labelRect = {20, y, SIDEBAR_WIDTH - 20, y + 18};
        Drawing::DrawText(hdc, L"URL Base:", labelRect, Colors::TextSecondary, 11);
        y += spacing;

        // Arquivo Patchlist
        labelRect = {20, y, SIDEBAR_WIDTH - 20, y + 18};
        Drawing::DrawText(hdc, L"Arquivo Patchlist:", labelRect, Colors::TextSecondary, 11);
        y += spacing;

        // Pasta de Patches
        labelRect = {20, y, SIDEBAR_WIDTH - 20, y + 18};
        Drawing::DrawText(hdc, L"Pasta de Patches:", labelRect, Colors::TextSecondary, 11);
        y += spacing + 5;

        // Secao: Jogo
        labelRect = {20, y, SIDEBAR_WIDTH - 20, y + 18};
        Drawing::DrawTextBold(hdc, L"CONFIGURACOES DO JOGO", labelRect, Colors::Accent, 10);
        y += 20;

        // GRF Principal
        labelRect = {20, y, SIDEBAR_WIDTH - 20, y + 18};
        Drawing::DrawText(hdc, L"GRF Principal:", labelRect, Colors::TextSecondary, 11);
        y += spacing;

        // Executavel do Jogo
        labelRect = {20, y, SIDEBAR_WIDTH - 20, y + 18};
        Drawing::DrawText(hdc, L"Executavel do Jogo:", labelRect, Colors::TextSecondary, 11);
        y += 50; // proximo label em y=555

        // Tamanho da Janela (y=555, edit em y=573)
        labelRect = {20, y, SIDEBAR_WIDTH - 20, y + 18};
        Drawing::DrawText(hdc, L"Tamanho da Janela:", labelRect, Colors::TextSecondary, 12);

        // X entre os campos de tamanho
        labelRect = {68, y + 21, 92, y + 39};
        Drawing::DrawText(hdc, L"x", labelRect, Colors::TextSecondary, 12);
        y += 55; // checkbox em y=610

        // Checkbox (y=610)
        m_chkCloseAfterStart->bounds = {20, y, SIDEBAR_WIDTH - 20, y + 24};
        m_chkCloseAfterStart->Paint(hdc, m_chkCloseAfterStart->bounds);
    }

    void ModernBuilderWindow::PaintMainArea(HDC hdc)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        // Area principal
        int mainLeft = SIDEBAR_WIDTH;
        int mainRight = rc.right - PROPERTIES_WIDTH;
        int mainTop = 70;
        int mainBottom = rc.bottom - FOOTER_HEIGHT;

        // Toolbar
        RECT toolbar = {mainLeft, mainTop, mainRight, mainTop + 50};
        Drawing::FillRect(hdc, toolbar, Colors::Secondary);

        // Titulo toolbar - muda conforme o modo
        RECT toolbarTitle = {mainLeft + 20, mainTop + 15, mainLeft + 350, mainTop + 40};
        if (m_project.interfaceMode == 0)
        {
            Drawing::DrawTextBold(hdc, L"Editor Visual - Modo Imagem", toolbarTitle, Colors::White, 14);

            // Botoes da toolbar (apenas no modo imagem)
            int btnX = mainRight - 540;
            int btnY = mainTop + 10;

            m_btnSelectBg->bounds = {btnX, btnY, btnX + 130, btnY + 30};
            m_btnSelectBg->isVisible = true;
            m_btnSelectBg->Paint(hdc, m_btnSelectBg->bounds);
            btnX += 135;

            m_btnAddButton->bounds = {btnX, btnY, btnX + 70, btnY + 30};
            m_btnAddButton->isVisible = true;
            m_btnAddButton->Paint(hdc, m_btnAddButton->bounds);
            btnX += 75;

            m_btnAddLabel->bounds = {btnX, btnY, btnX + 65, btnY + 30};
            m_btnAddLabel->isVisible = true;
            m_btnAddLabel->Paint(hdc, m_btnAddLabel->bounds);
            btnX += 70;

            m_btnAddProgress->bounds = {btnX, btnY, btnX + 45, btnY + 30};
            m_btnAddProgress->isVisible = true;
            m_btnAddProgress->Paint(hdc, m_btnAddProgress->bounds);
            btnX += 50;

            m_btnAddProgressBar->bounds = {btnX, btnY, btnX + 60, btnY + 30};
            m_btnAddProgressBar->isVisible = true;
            m_btnAddProgressBar->Paint(hdc, m_btnAddProgressBar->bounds);

            // Canvas
            RECT canvasArea = {mainLeft + 20, mainTop + 60, mainRight - 20, mainBottom - 20};
            m_canvas->bounds = canvasArea;
            m_canvas->Paint(hdc, canvasArea);
        }
        else
        {
            Drawing::DrawTextBold(hdc, L"Editor HTML/CSS/JS", toolbarTitle, Colors::White, 14);

            // Esconde botoes do modo imagem
            m_btnSelectBg->isVisible = false;
            m_btnAddButton->isVisible = false;
            m_btnAddLabel->isVisible = false;
            m_btnAddProgress->isVisible = false;
            m_btnAddProgressBar->isVisible = false;

            // Area de texto para HTML
            RECT htmlArea = {mainLeft + 20, mainTop + 60, mainRight - 20, mainBottom - 20};
            Drawing::FillRect(hdc, htmlArea, Colors::Primary);
            Drawing::DrawRect(hdc, htmlArea, Colors::Border, 1);

            // Instrucoes
            RECT infoRect = {mainLeft + 40, mainTop + 100, mainRight - 40, mainBottom - 40};
            Drawing::DrawText(hdc,
                              L"Modo HTML/CSS/JS\n\n"
                              L"Neste modo, voce pode criar uma interface web personalizada\n"
                              L"usando HTML, CSS e JavaScript.\n\n"
                              L"A interface sera carregada dentro do patcher usando\n"
                              L"um navegador embutido (WebView).\n\n"
                              L"Funcionalidade em desenvolvimento...",
                              infoRect, Colors::TextSecondary, 13, L"Segoe UI", DT_CENTER | DT_WORDBREAK);
        }

        // Properties panel
        PaintPropertiesPanel(hdc);
    }

    void ModernBuilderWindow::PaintPropertiesPanel(HDC hdc)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        int panelLeft = rc.right - PROPERTIES_WIDTH;
        int panelTop = 70;
        int panelBottom = rc.bottom - FOOTER_HEIGHT;

        // Background
        RECT panel = {panelLeft, panelTop, rc.right, panelBottom};
        Drawing::FillRect(hdc, panel, Colors::Secondary);

        // Titulo
        RECT titleRect = {panelLeft + 16, panelTop + 16, rc.right - 16, panelTop + 40};
        Drawing::DrawTextBold(hdc, L"Propriedades", titleRect, Colors::White, 14);

        auto *selected = m_canvas ? m_canvas->selectedElement : nullptr;

        // Esconde ou mostra controles de propriedade
        bool showProps = (selected != nullptr);
        ShowWindow(m_editElemX, showProps ? SW_SHOW : SW_HIDE);
        ShowWindow(m_editElemY, showProps ? SW_SHOW : SW_HIDE);
        ShowWindow(m_editElemW, showProps ? SW_SHOW : SW_HIDE);
        ShowWindow(m_editElemH, showProps ? SW_SHOW : SW_HIDE);
        ShowWindow(m_editElemText, showProps && selected && selected->type != 2 ? SW_SHOW : SW_HIDE);
        ShowWindow(m_comboElemAction, showProps && selected && selected->type == 0 ? SW_SHOW : SW_HIDE);

        if (!selected)
        {
            RECT noSelRect = {panelLeft + 16, panelTop + 60, rc.right - 16, panelTop + 100};
            Drawing::DrawText(hdc, L"Selecione um elemento\nno canvas para editar",
                              noSelRect, Colors::TextSecondary, 12, L"Segoe UI", DT_CENTER | DT_WORDBREAK);
            return;
        }

        int y = panelTop + 55;
        int labelX = panelLeft + 16;
        int editX = panelLeft + 16;
        int editW = (PROPERTIES_WIDTH - 48) / 2;

        // Tipo
        RECT typeRect = {labelX, y, rc.right - 16, y + 20};
        std::wstring typeStr = selected->type == 0 ? L"Botao" : (selected->type == 1 ? L"Label" : L"ProgressBar");
        Drawing::DrawText(hdc, L"Tipo: " + typeStr, typeRect, Colors::White, 12);
        y += 28;

        // Posicao (labels)
        RECT posLabel = {labelX, y, rc.right - 16, y + 18};
        Drawing::DrawText(hdc, L"Posicao X / Y:", posLabel, Colors::TextSecondary, 11);
        y += 20;

        // Reposiciona os edits de posicao
        MoveWindow(m_editElemX, editX, y, editW, 22, TRUE);
        MoveWindow(m_editElemY, editX + editW + 8, y, editW, 22, TRUE);
        y += 32;

        // Tamanho (labels)
        RECT sizeLabel = {labelX, y, rc.right - 16, y + 18};
        Drawing::DrawText(hdc, L"Largura / Altura:", sizeLabel, Colors::TextSecondary, 11);
        y += 20;

        // Reposiciona os edits de tamanho
        MoveWindow(m_editElemW, editX, y, editW, 22, TRUE);
        MoveWindow(m_editElemH, editX + editW + 8, y, editW, 22, TRUE);
        y += 32;

        // Texto (para botoes e labels)
        if (selected->type != 2)
        {
            RECT textLabel = {labelX, y, rc.right - 16, y + 18};
            Drawing::DrawText(hdc, L"Texto:", textLabel, Colors::TextSecondary, 11);
            y += 20;

            MoveWindow(m_editElemText, editX, y, PROPERTIES_WIDTH - 32, 22, TRUE);
            y += 32;
        }

        // Acao (para botoes)
        if (selected->type == 0)
        {
            RECT actionLabel = {labelX, y, rc.right - 16, y + 18};
            Drawing::DrawText(hdc, L"Acao:", actionLabel, Colors::TextSecondary, 11);
            y += 20;

            MoveWindow(m_comboElemAction, editX, y, PROPERTIES_WIDTH - 32, 200, TRUE);
            y += 32;
        }

        // Botao remover
        y += 15;
        m_btnRemoveElement->bounds = {labelX, y, rc.right - 16, y + 30};
        m_btnRemoveElement->isVisible = true;
        m_btnRemoveElement->Paint(hdc, m_btnRemoveElement->bounds);
    }

    void ModernBuilderWindow::PaintFooter(HDC hdc)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        // Background
        RECT footer = {0, rc.bottom - FOOTER_HEIGHT, rc.right, rc.bottom};
        Drawing::FillRect(hdc, footer, Colors::Secondary);

        // Status
        RECT statusRect = {20, rc.bottom - 40, 400, rc.bottom - 15};
        Drawing::DrawText(hdc, L"✓ " + m_statusText, statusRect, Colors::TextSecondary, 12);

        // Botões
        int btnY = rc.bottom - 45;

        // Open/Save
        int btnX = 20;
        m_btnOpenProject->bounds = {btnX, btnY, btnX + 130, btnY + 35};
        m_btnOpenProject->Paint(hdc, m_btnOpenProject->bounds);
        btnX += 140;

        m_btnSaveProject->bounds = {btnX, btnY, btnX + 130, btnY + 35};
        m_btnSaveProject->Paint(hdc, m_btnSaveProject->bounds);

        // Preview & Generate
        btnX = rc.right - 320;
        m_btnPreview->bounds = {btnX, btnY, btnX + 140, btnY + 35};
        m_btnPreview->Paint(hdc, m_btnPreview->bounds);
        btnX += 150;

        m_btnGenerate->bounds = {btnX, btnY, btnX + 160, btnY + 35};
        m_btnGenerate->Paint(hdc, m_btnGenerate->bounds);
    }

    void ModernBuilderWindow::OnCommand(WPARAM wParam, LPARAM lParam)
    {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        if (code == EN_CHANGE)
        {
            wchar_t buf[512];
            switch (id)
            {
            case ID_EDIT_SERVER_NAME:
                GetWindowTextW(m_editServerName, buf, 512);
                m_project.serverName = buf;
                break;
            case ID_EDIT_BASE_URL:
                GetWindowTextW(m_editBaseUrl, buf, 512);
                m_project.baseUrl = buf;
                break;
            case ID_EDIT_PATCHLIST:
                GetWindowTextW(m_editPatchlist, buf, 512);
                m_project.patchlistFile = buf;
                break;
            case ID_EDIT_PATCHES_FOLDER:
                GetWindowTextW(m_editPatchesFolder, buf, 512);
                m_project.patchesFolder = buf;
                break;
            case ID_EDIT_MAIN_GRF:
                GetWindowTextW(m_editMainGrf, buf, 512);
                m_project.mainGrf = buf;
                break;
            case ID_EDIT_GAME_EXE:
                GetWindowTextW(m_editGameExe, buf, 512);
                m_project.gameExecutable = buf;
                break;
            case ID_EDIT_WINDOW_WIDTH:
                GetWindowTextW(m_editWindowWidth, buf, 512);
                m_project.windowWidth = _wtoi(buf);
                if (m_project.windowWidth < 100)
                    m_project.windowWidth = 100;
                Invalidate();
                break;
            case ID_EDIT_WINDOW_HEIGHT:
                GetWindowTextW(m_editWindowHeight, buf, 512);
                m_project.windowHeight = _wtoi(buf);
                if (m_project.windowHeight < 100)
                    m_project.windowHeight = 100;
                Invalidate();
                break;
            // Campos de propriedade do elemento - nao redesenha a cada tecla
            case ID_EDIT_ELEM_X:
            case ID_EDIT_ELEM_Y:
            case ID_EDIT_ELEM_W:
            case ID_EDIT_ELEM_H:
                ApplyPropertyChanges();
                break;
            case ID_EDIT_ELEM_TEXT:
                // Apenas atualiza o texto sem redesenhar
                if (m_canvas && m_canvas->selectedElement)
                {
                    GetWindowTextW(m_editElemText, buf, 512);
                    m_canvas->selectedElement->text = buf;
                }
                break;
            }
        }
        else if (code == CBN_SELCHANGE)
        {
            if (id == ID_COMBO_ELEM_ACTION)
            {
                ApplyPropertyChanges();
            }
        }
    }

    void ModernBuilderWindow::OnMouseMove(int x, int y)
    {
        bool needRepaint = false;

        // Update hover states
        auto updateHover = [&](CustomControl *ctrl)
        {
            if (!ctrl)
                return;
            bool wasHovered = ctrl->isHovered;
            ctrl->isHovered = ctrl->HitTest(x, y);
            if (wasHovered != ctrl->isHovered)
                needRepaint = true;
        };

        updateHover(m_imageModeCard.get());
        updateHover(m_htmlModeCard.get());
        updateHover(m_btnSelectBg.get());
        updateHover(m_btnAddButton.get());
        updateHover(m_btnAddLabel.get());
        updateHover(m_btnAddProgress.get());
        updateHover(m_btnAddProgressBar.get());
        updateHover(m_btnRemoveElement.get());
        updateHover(m_btnPreview.get());
        updateHover(m_btnGenerate.get());
        updateHover(m_btnOpenProject.get());
        updateHover(m_btnSaveProject.get());
        updateHover(m_chkCloseAfterStart.get());

        // Canvas drag - repaint apenas se estiver arrastando
        if (m_canvas && m_canvas->isDragging)
        {
            m_canvas->OnMouseMove(x, y);
            needRepaint = true;
        }
        else if (m_canvas)
        {
            m_canvas->OnMouseMove(x, y);
        }

        if (needRepaint)
        {
            // Usa região específica do canvas para evitar redesenhar tudo
            InvalidateRect(m_hwnd, &m_canvas->bounds, FALSE);
        }
    }

    void ModernBuilderWindow::OnMouseDown(int x, int y)
    {
        if (m_imageModeCard && m_imageModeCard->HitTest(x, y))
            m_imageModeCard->OnMouseUp(x, y);
        else if (m_htmlModeCard && m_htmlModeCard->HitTest(x, y))
            m_htmlModeCard->OnMouseUp(x, y);
        else if (m_btnSelectBg && m_btnSelectBg->HitTest(x, y))
            m_btnSelectBg->OnMouseDown(x, y);
        else if (m_btnAddButton && m_btnAddButton->HitTest(x, y))
            m_btnAddButton->OnMouseDown(x, y);
        else if (m_btnAddLabel && m_btnAddLabel->HitTest(x, y))
            m_btnAddLabel->OnMouseDown(x, y);
        else if (m_btnAddProgress && m_btnAddProgress->HitTest(x, y))
            m_btnAddProgress->OnMouseDown(x, y);
        else if (m_btnAddProgressBar && m_btnAddProgressBar->HitTest(x, y))
            m_btnAddProgressBar->OnMouseDown(x, y);
        else if (m_btnRemoveElement && m_btnRemoveElement->HitTest(x, y))
            m_btnRemoveElement->OnMouseDown(x, y);
        else if (m_btnPreview && m_btnPreview->HitTest(x, y))
            m_btnPreview->OnMouseDown(x, y);
        else if (m_btnGenerate && m_btnGenerate->HitTest(x, y))
            m_btnGenerate->OnMouseDown(x, y);
        else if (m_btnOpenProject && m_btnOpenProject->HitTest(x, y))
            m_btnOpenProject->OnMouseDown(x, y);
        else if (m_btnSaveProject && m_btnSaveProject->HitTest(x, y))
            m_btnSaveProject->OnMouseDown(x, y);
        else if (m_chkCloseAfterStart && m_chkCloseAfterStart->HitTest(x, y))
            m_chkCloseAfterStart->OnMouseUp(x, y);
        else if (m_canvas && m_canvas->HitTest(x, y))
            m_canvas->OnMouseDown(x, y);

        Invalidate();
    }

    void ModernBuilderWindow::OnMouseUp(int x, int y)
    {
        if (m_btnSelectBg)
            m_btnSelectBg->OnMouseUp(x, y);
        if (m_btnAddButton)
            m_btnAddButton->OnMouseUp(x, y);
        if (m_btnAddLabel)
            m_btnAddLabel->OnMouseUp(x, y);
        if (m_btnAddProgress)
            m_btnAddProgress->OnMouseUp(x, y);
        if (m_btnAddProgressBar)
            m_btnAddProgressBar->OnMouseUp(x, y);
        if (m_btnRemoveElement)
            m_btnRemoveElement->OnMouseUp(x, y);
        if (m_btnPreview)
            m_btnPreview->OnMouseUp(x, y);
        if (m_btnGenerate)
            m_btnGenerate->OnMouseUp(x, y);
        if (m_btnOpenProject)
            m_btnOpenProject->OnMouseUp(x, y);
        if (m_btnSaveProject)
            m_btnSaveProject->OnMouseUp(x, y);
        if (m_canvas)
            m_canvas->OnMouseUp(x, y);

        Invalidate();
    }

    void ModernBuilderWindow::OnSize(int width, int height)
    {
        m_width = width;
        m_height = height;

        // Reposiciona edits na sidebar (ficam em posições fixas)
        // Os edits já foram criados em posições fixas, não precisamos mover

        Invalidate();
    }

    void ModernBuilderWindow::SelectImageMode()
    {
        m_project.interfaceMode = 0;
        m_imageModeCard->isSelected = true;
        m_htmlModeCard->isSelected = false;
        Invalidate();
    }

    void ModernBuilderWindow::SelectHtmlMode()
    {
        m_project.interfaceMode = 1;
        m_imageModeCard->isSelected = false;
        m_htmlModeCard->isSelected = true;
        Invalidate();
    }

    void ModernBuilderWindow::SelectBackgroundImage()
    {
        auto path = OpenFileDialog(
            L"Imagens (*.png;*.jpg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0Todos (*.*)\0*.*\0",
            L"Selecionar Background");

        if (!path.empty())
        {
            m_project.backgroundImagePath = path;
            m_canvas->LoadBackgroundImage(path);

            // Atualiza os campos de tamanho da janela com as dimensões da imagem
            SetWindowTextW(m_editWindowWidth, std::to_wstring(m_project.windowWidth).c_str());
            SetWindowTextW(m_editWindowHeight, std::to_wstring(m_project.windowHeight).c_str());

            SetStatus(L"Background carregado: " + path.substr(path.find_last_of(L"\\") + 1) +
                      L" (" + std::to_wstring(m_project.windowWidth) + L"x" +
                      std::to_wstring(m_project.windowHeight) + L")");
            Invalidate();
        }
    }

    // Dialog para adicionar elemento
    struct AddElementDialogData
    {
        int type;
        std::wstring text;
        int x, y, width, height;
        std::string action;
        bool confirmed;
        HWND hCombo;
    };

    static AddElementDialogData *s_dlgData = nullptr;

    static LRESULT CALLBACK AddElementDlgWndProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_COMMAND:
        {
            int cmd = LOWORD(wParam);
            if (cmd == IDOK && s_dlgData)
            {
                // Coleta dados
                wchar_t buf[256];
                if (s_dlgData->type != 2)
                {
                    GetDlgItemTextW(hDlg, 101, buf, 256);
                    s_dlgData->text = buf;
                }
                s_dlgData->x = GetDlgItemInt(hDlg, 102, nullptr, FALSE);
                s_dlgData->y = GetDlgItemInt(hDlg, 103, nullptr, FALSE);
                s_dlgData->width = GetDlgItemInt(hDlg, 104, nullptr, FALSE);
                s_dlgData->height = GetDlgItemInt(hDlg, 105, nullptr, FALSE);

                if (s_dlgData->type == 0 && s_dlgData->hCombo)
                {
                    int idx = (int)SendMessageW(s_dlgData->hCombo, CB_GETCURSEL, 0, 0);
                    if (idx != CB_ERR)
                    {
                        SendMessageW(s_dlgData->hCombo, CB_GETLBTEXT, idx, (LPARAM)buf);
                        std::wstring actionW = buf;
                        // Convert wstring to string properly
                        int size_needed = WideCharToMultiByte(CP_UTF8, 0, actionW.c_str(), (int)actionW.size(), NULL, 0, NULL, NULL);
                        s_dlgData->action.resize(size_needed);
                        WideCharToMultiByte(CP_UTF8, 0, actionW.c_str(), (int)actionW.size(), &s_dlgData->action[0], size_needed, NULL, NULL);
                    }
                }
                s_dlgData->confirmed = true;
                PostMessage(hDlg, WM_CLOSE, 0, 0);
                return 0;
            }
            else if (cmd == IDCANCEL)
            {
                s_dlgData->confirmed = false;
                PostMessage(hDlg, WM_CLOSE, 0, 0);
                return 0;
            }
            break;
        }
        case WM_CLOSE:
            DestroyWindow(hDlg);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hDlg, msg, wParam, lParam);
    }

    void ModernBuilderWindow::AddElement(int type)
    {
        AddElementDialogData data;
        data.type = type;
        data.confirmed = false;
        data.hCombo = nullptr;

        // Valores padrao
        switch (type)
        {
        case 0: // Button
            data.text = L"Iniciar";
            data.width = 120;
            data.height = 35;
            data.action = "start_game";
            break;
        case 1: // Label generico
            data.text = L"Texto";
            data.width = 200;
            data.height = 24;
            break;
        case 2: // ProgressBar
            data.text = L"";
            data.width = 400;
            data.height = 20;
            break;
        case 3: // Label Status (ID=1)
            data.text = L"Arquivos atualizados!";
            data.width = 300;
            data.height = 24;
            break;
        case 4: // Label Percentagem (ID=2)
            data.text = L"100%";
            data.width = 80;
            data.height = 24;
            break;
        }
        data.x = 50;
        data.y = 50;

        s_dlgData = &data;

        // Registra classe do dialog
        WNDCLASSEXW wcDlg = {};
        wcDlg.cbSize = sizeof(wcDlg);
        wcDlg.lpfnWndProc = AddElementDlgWndProc;
        wcDlg.hInstance = m_hInstance;
        wcDlg.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcDlg.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        wcDlg.lpszClassName = L"AddElementDlg";
        RegisterClassExW(&wcDlg);

        // Calcula altura do dialog
        int dlgHeight = type == 0 ? 300 : (type == 2 ? 220 : 250);

        // Titulo do dialog baseado no tipo
        const wchar_t *dlgTitle;
        switch (type)
        {
        case 0:
            dlgTitle = L"Adicionar Botao";
            break;
        case 1:
            dlgTitle = L"Adicionar Label";
            break;
        case 2:
            dlgTitle = L"Adicionar ProgressBar";
            break;
        case 3:
            dlgTitle = L"Adicionar Label de Status";
            break;
        case 4:
            dlgTitle = L"Adicionar Label de Percentagem";
            break;
        default:
            dlgTitle = L"Adicionar Elemento";
            break;
        }

        // Cria janela do dialog
        HWND hDlg = CreateWindowExW(
            WS_EX_DLGMODALFRAME,
            L"AddElementDlg",
            dlgTitle,
            WS_POPUP | WS_CAPTION | WS_SYSMENU,
            0, 0, 320, dlgHeight,
            m_hwnd, nullptr, m_hInstance, nullptr);

        if (!hDlg)
        {
            s_dlgData = nullptr;
            return;
        }

        // Centraliza
        RECT parentRc;
        GetWindowRect(m_hwnd, &parentRc);
        int dlgX = parentRc.left + (parentRc.right - parentRc.left - 320) / 2;
        int dlgY = parentRc.top + (parentRc.bottom - parentRc.top - dlgHeight) / 2;
        SetWindowPos(hDlg, nullptr, dlgX, dlgY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        HFONT hFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        int y = 15;
        auto createLabel = [&](const wchar_t *text, int id)
        {
            HWND h = CreateWindowExW(0, L"STATIC", text,
                                     WS_CHILD | WS_VISIBLE | SS_LEFT,
                                     15, y + 3, 80, 20, hDlg, (HMENU)(INT_PTR)id, m_hInstance, nullptr);
            SendMessage(h, WM_SETFONT, (WPARAM)hFont, TRUE);
        };
        auto createEdit = [&](const wchar_t *text, int id, int w = 180)
        {
            HWND h = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", text,
                                     WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                     100, y, w, 24, hDlg, (HMENU)(INT_PTR)id, m_hInstance, nullptr);
            SendMessage(h, WM_SETFONT, (WPARAM)hFont, TRUE);
            y += 32;
        };

        // Texto (nao para ProgressBar)
        if (type != 2)
        {
            createLabel(L"Texto:", 201);
            createEdit(data.text.c_str(), 101, 190);
        }

        // Posicao X
        createLabel(L"Posicao X:", 202);
        createEdit(std::to_wstring(data.x).c_str(), 102, 80);

        // Posicao Y
        createLabel(L"Posicao Y:", 203);
        createEdit(std::to_wstring(data.y).c_str(), 103, 80);

        // Largura
        createLabel(L"Largura:", 204);
        createEdit(std::to_wstring(data.width).c_str(), 104, 80);

        // Altura
        createLabel(L"Altura:", 205);
        createEdit(std::to_wstring(data.height).c_str(), 105, 80);

        // Acao (apenas para Botao)
        if (type == 0)
        {
            createLabel(L"Acao:", 206);
            data.hCombo = CreateWindowExW(0, L"COMBOBOX", L"",
                                          WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                                          100, y, 190, 150, hDlg, (HMENU)106, m_hInstance, nullptr);
            SendMessage(data.hCombo, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessageW(data.hCombo, CB_ADDSTRING, 0, (LPARAM)L"start_game");
            SendMessageW(data.hCombo, CB_ADDSTRING, 0, (LPARAM)L"check_updates");
            SendMessageW(data.hCombo, CB_ADDSTRING, 0, (LPARAM)L"exit");
            SendMessageW(data.hCombo, CB_ADDSTRING, 0, (LPARAM)L"open_url");
            SendMessageW(data.hCombo, CB_SETCURSEL, 0, 0);
            s_dlgData->hCombo = data.hCombo;
            y += 32;
        }

        // Botoes OK e Cancelar
        y += 10;
        HWND hOk = CreateWindowExW(0, L"BUTTON", L"Adicionar",
                                   WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                   60, y, 90, 28, hDlg, (HMENU)IDOK, m_hInstance, nullptr);
        SendMessage(hOk, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hCancel = CreateWindowExW(0, L"BUTTON", L"Cancelar",
                                       WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                       160, y, 90, 28, hDlg, (HMENU)IDCANCEL, m_hInstance, nullptr);
        SendMessage(hCancel, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Mostra dialog modal
        ShowWindow(hDlg, SW_SHOW);
        UpdateWindow(hDlg);
        EnableWindow(m_hwnd, FALSE);

        // Message loop proprio para o dialog
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            if (!IsWindow(hDlg))
                break;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        EnableWindow(m_hwnd, TRUE);
        SetForegroundWindow(m_hwnd);
        DeleteObject(hFont);

        bool confirmed = data.confirmed;
        s_dlgData = nullptr;

        if (confirmed)
        {
            UIElementData elem;
            elem.type = type;
            elem.x = data.x;
            elem.y = data.y;
            elem.width = data.width;
            elem.height = data.height;
            elem.text = data.text;
            elem.action = data.action;

            m_project.elements.push_back(elem);
            m_canvas->selectedElement = &m_project.elements.back();
            SetStatus(L"Elemento adicionado");
            Invalidate();
        }

        UnregisterClassW(L"AddElementDlg", m_hInstance);
    }

    void ModernBuilderWindow::RemoveSelectedElement()
    {
        auto *selected = m_canvas->selectedElement;
        if (!selected)
            return;

        auto &elems = m_project.elements;
        for (auto it = elems.begin(); it != elems.end(); ++it)
        {
            if (&(*it) == selected)
            {
                elems.erase(it);
                break;
            }
        }
        m_canvas->selectedElement = nullptr;
        SetStatus(L"Elemento removido");
        UpdatePropertiesPanel();
        Invalidate();
    }

    void ModernBuilderWindow::UpdatePropertiesPanel()
    {
        auto *selected = m_canvas ? m_canvas->selectedElement : nullptr;

        // Evita loop de atualização
        m_updatingProperties = true;

        if (selected)
        {
            // Preenche os campos com os valores do elemento
            SetWindowTextW(m_editElemX, std::to_wstring(selected->x).c_str());
            SetWindowTextW(m_editElemY, std::to_wstring(selected->y).c_str());
            SetWindowTextW(m_editElemW, std::to_wstring(selected->width).c_str());
            SetWindowTextW(m_editElemH, std::to_wstring(selected->height).c_str());
            SetWindowTextW(m_editElemText, selected->text.c_str());

            // Seleciona a acao no combo
            if (selected->type == 0)
            {
                std::wstring actionW(selected->action.begin(), selected->action.end());
                int idx = (int)SendMessageW(m_comboElemAction, CB_FINDSTRINGEXACT, -1, (LPARAM)actionW.c_str());
                if (idx != CB_ERR)
                    SendMessageW(m_comboElemAction, CB_SETCURSEL, idx, 0);
            }
        }

        m_updatingProperties = false;
        Invalidate();
    }

    void ModernBuilderWindow::ApplyPropertyChanges()
    {
        // Ignora se estamos atualizando os campos programaticamente
        if (m_updatingProperties)
            return;

        auto *selected = m_canvas ? m_canvas->selectedElement : nullptr;
        if (!selected)
            return;

        // Le os valores dos campos
        wchar_t buf[256];

        GetWindowTextW(m_editElemX, buf, 256);
        selected->x = _wtoi(buf);

        GetWindowTextW(m_editElemY, buf, 256);
        selected->y = _wtoi(buf);

        GetWindowTextW(m_editElemW, buf, 256);
        selected->width = _wtoi(buf);

        GetWindowTextW(m_editElemH, buf, 256);
        selected->height = _wtoi(buf);

        if (selected->type != 2)
        {
            GetWindowTextW(m_editElemText, buf, 256);
            selected->text = buf;
        }

        if (selected->type == 0)
        {
            int idx = (int)SendMessageW(m_comboElemAction, CB_GETCURSEL, 0, 0);
            if (idx != CB_ERR)
            {
                SendMessageW(m_comboElemAction, CB_GETLBTEXT, idx, (LPARAM)buf);
                std::wstring actionW = buf;
                selected->action = utils::WideToString(actionW);
            }
        }

        Invalidate();
    }

    void ModernBuilderWindow::OpenProject()
    {
        auto path = OpenFileDialog(
            L"Projeto AutoPatch (*.approj)\0*.approj\0Todos (*.*)\0*.*\0",
            L"Abrir Projeto");

        if (path.empty())
            return;

        try
        {
            std::ifstream file(path);
            json j;
            file >> j;

            m_project.serverName = utils::StringToWide(j.value("serverName", ""));
            m_project.baseUrl = utils::StringToWide(j.value("baseUrl", ""));
            m_project.patchlistFile = utils::StringToWide(j.value("patchlistFile", ""));
            m_project.patchesFolder = utils::StringToWide(j.value("patchesFolder", ""));
            m_project.mainGrf = utils::StringToWide(j.value("mainGrf", ""));
            m_project.gameExecutable = utils::StringToWide(j.value("gameExecutable", ""));
            m_project.windowWidth = j.value("windowWidth", 800);
            m_project.windowHeight = j.value("windowHeight", 600);
            m_project.closeAfterStart = j.value("closeAfterStart", true);
            m_project.interfaceMode = j.value("interfaceMode", 0);

            // Update UI
            SetWindowTextW(m_editServerName, m_project.serverName.c_str());
            SetWindowTextW(m_editBaseUrl, m_project.baseUrl.c_str());
            SetWindowTextW(m_editPatchlist, m_project.patchlistFile.c_str());
            SetWindowTextW(m_editPatchesFolder, m_project.patchesFolder.c_str());
            SetWindowTextW(m_editMainGrf, m_project.mainGrf.c_str());
            SetWindowTextW(m_editGameExe, m_project.gameExecutable.c_str());
            SetWindowTextW(m_editWindowWidth, std::to_wstring(m_project.windowWidth).c_str());
            SetWindowTextW(m_editWindowHeight, std::to_wstring(m_project.windowHeight).c_str());
            m_chkCloseAfterStart->isChecked = m_project.closeAfterStart;

            m_projectPath = path;
            SetStatus(L"Projeto carregado: " + path.substr(path.find_last_of(L"\\") + 1));
            Invalidate();
        }
        catch (const std::exception &e)
        {
            MessageBoxA(m_hwnd, e.what(), "Erro ao carregar projeto", MB_ICONERROR);
        }
    }

    void ModernBuilderWindow::SaveProject()
    {
        std::wstring path = m_projectPath;
        if (path.empty())
        {
            path = SaveFileDialog(
                L"Projeto AutoPatch (*.approj)\0*.approj\0",
                L"Salvar Projeto",
                L"projeto.approj");
            if (path.empty())
                return;
        }

        try
        {
            json j;
            j["serverName"] = utils::WideToString(m_project.serverName);
            j["baseUrl"] = utils::WideToString(m_project.baseUrl);
            j["patchlistFile"] = utils::WideToString(m_project.patchlistFile);
            j["patchesFolder"] = utils::WideToString(m_project.patchesFolder);
            j["mainGrf"] = utils::WideToString(m_project.mainGrf);
            j["gameExecutable"] = utils::WideToString(m_project.gameExecutable);
            j["windowWidth"] = m_project.windowWidth;
            j["windowHeight"] = m_project.windowHeight;
            j["closeAfterStart"] = m_project.closeAfterStart;
            j["interfaceMode"] = m_project.interfaceMode;

            std::ofstream file(path);
            file << j.dump(2);

            m_projectPath = path;
            SetStatus(L"Projeto salvo: " + path.substr(path.find_last_of(L"\\") + 1));
        }
        catch (const std::exception &e)
        {
            MessageBoxA(m_hwnd, e.what(), "Erro ao salvar projeto", MB_ICONERROR);
        }
    }

    void ModernBuilderWindow::Preview()
    {
        SetStatus(L"Preview: funcionalidade em desenvolvimento");
    }

    void ModernBuilderWindow::ShowExportDialog()
    {
        auto outputPath = SaveFileDialog(
            L"Executável (*.exe)\0*.exe\0",
            L"Salvar Patcher",
            L"patcher.exe");

        if (outputPath.empty())
            return;

        GenerateExe();
    }

    void ModernBuilderWindow::GenerateExe()
    {
        // Procura template
        wchar_t modulePath[MAX_PATH];
        GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
        std::wstring dir = modulePath;
        dir = dir.substr(0, dir.find_last_of(L"\\"));

        std::wstring templatePath = dir + L"\\AutoPatcher.exe";

        if (GetFileAttributesW(templatePath.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            MessageBoxW(m_hwnd,
                        L"Template AutoPatcher.exe não encontrado!\n\n"
                        L"Coloque o AutoPatcher.exe na mesma pasta do Builder.",
                        L"Erro", MB_ICONERROR);
            return;
        }

        std::wstring defaultName = m_project.serverName + L"_patcher.exe";
        auto outputPath = SaveFileDialog(
            L"Executável (*.exe)\0*.exe\0",
            L"Salvar Patcher",
            defaultName.c_str());

        if (outputPath.empty())
            return;

        SetStatus(L"Gerando patcher...");

        if (EmbedConfigInExe(templatePath, outputPath))
        {
            SetStatus(L"Patcher gerado com sucesso!");
            MessageBoxW(m_hwnd,
                        (L"Patcher gerado com sucesso!\n\n" + outputPath).c_str(),
                        L"Sucesso", MB_ICONINFORMATION);

            // Abre pasta
            ShellExecuteW(nullptr, L"open", L"explorer.exe",
                          (L"/select,\"" + outputPath + L"\"").c_str(), nullptr, SW_SHOW);
        }
        else
        {
            SetStatus(L"Erro ao gerar patcher");
            MessageBoxW(m_hwnd, L"Erro ao gerar patcher!", L"Erro", MB_ICONERROR);
        }
    }

    bool ModernBuilderWindow::EmbedConfigInExe(const std::wstring &templatePath, const std::wstring &outputPath)
    {
        // Copia template para destino
        if (!CopyFileW(templatePath.c_str(), outputPath.c_str(), FALSE))
            return false;

        // Gera JSON de configuração
        std::string configJson = GenerateConfigJson();

        // Usa API do Windows para embutir como recurso
        HANDLE hUpdate = BeginUpdateResourceW(outputPath.c_str(), FALSE);
        if (!hUpdate)
        {
            DeleteFileW(outputPath.c_str());
            return false;
        }

        // Embutir como recurso RCDATA com ID 1001 (deve corresponder ao config.cpp)
        constexpr int ID_CONFIG = 1001;
        constexpr int ID_BACKGROUND = 1003;

        BOOL result = UpdateResourceW(hUpdate,
                                      RT_RCDATA,
                                      MAKEINTRESOURCEW(ID_CONFIG),
                                      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                      (LPVOID)configJson.data(),
                                      (DWORD)configJson.size());

        // Embutir imagem de fundo se existir
        if (result && !m_project.backgroundImagePath.empty())
        {
            std::vector<uint8_t> imgData = utils::ReadAllBytes(m_project.backgroundImagePath);
            if (!imgData.empty())
            {
                UpdateResourceW(hUpdate,
                                RT_RCDATA,
                                MAKEINTRESOURCEW(ID_BACKGROUND),
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                (LPVOID)imgData.data(),
                                (DWORD)imgData.size());
            }
        }

        if (!result)
        {
            EndUpdateResourceW(hUpdate, TRUE); // Descarta
            DeleteFileW(outputPath.c_str());
            return false;
        }

        // Finaliza
        if (!EndUpdateResourceW(hUpdate, FALSE))
        {
            DeleteFileW(outputPath.c_str());
            return false;
        }

        return true;
    }

    std::string ModernBuilderWindow::GenerateConfigJson()
    {
        json config;
        config["serverName"] = utils::WideToString(m_project.serverName);
        config["patchListUrl"] = utils::WideToString(m_project.baseUrl + m_project.patchlistFile);
        config["patchesUrl"] = utils::WideToString(m_project.baseUrl + m_project.patchesFolder);
        config["clientExe"] = utils::WideToString(m_project.gameExecutable);
        config["clientArgs"] = utils::WideToString(m_project.gameArguments);
        config["grfFiles"] = json::array({utils::WideToString(m_project.mainGrf)});
        config["windowWidth"] = m_project.windowWidth;
        config["windowHeight"] = m_project.windowHeight;
        config["closeAfterStart"] = m_project.closeAfterStart;
        config["uiType"] = m_project.interfaceMode;

        // Elementos UI
        if (m_project.interfaceMode == 0)
        {
            json imageMode;
            imageMode["backgroundImage"] = ""; // TODO: encode base64

            json buttons = json::array();
            json labels = json::array();
            json progressBars = json::array();

            for (const auto &elem : m_project.elements)
            {
                json e;
                e["x"] = elem.x;
                e["y"] = elem.y;
                e["width"] = elem.width;
                e["height"] = elem.height;
                e["text"] = utils::WideToString(elem.text);

                if (elem.type == 0) // Botao
                {
                    e["id"] = elem.id.empty() ? "btn_" + std::to_string(buttons.size()) : elem.id;
                    e["action"] = elem.action;
                    buttons.push_back(e);
                }
                else if (elem.type == 1) // Label generico
                {
                    e["id"] = elem.id.empty() ? "label_" + std::to_string(labels.size()) : elem.id;
                    labels.push_back(e);
                }
                else if (elem.type == 2) // ProgressBar
                {
                    progressBars.push_back(e);
                }
                else if (elem.type == 3) // Status Label (ID fixo = 1)
                {
                    e["id"] = "1"; // ID especial para status
                    labels.push_back(e);
                }
                else if (elem.type == 4) // Percentagem Label (ID fixo = 2)
                {
                    e["id"] = "2"; // ID especial para percentagem
                    labels.push_back(e);
                }
            }

            imageMode["buttons"] = buttons;
            imageMode["labels"] = labels;
            if (!progressBars.empty())
                imageMode["progressBar"] = progressBars[0];

            config["imageMode"] = imageMode;
        }

        return config.dump();
    }

    void ModernBuilderWindow::SetStatus(const std::wstring &text)
    {
        m_statusText = text;
        Invalidate();
    }

    void ModernBuilderWindow::Invalidate()
    {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    std::wstring ModernBuilderWindow::OpenFileDialog(const wchar_t *filter, const wchar_t *title)
    {
        wchar_t filename[MAX_PATH] = {};
        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_hwnd;
        ofn.lpstrFilter = filter;
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = title;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        if (GetOpenFileNameW(&ofn))
            return filename;
        return L"";
    }

    std::wstring ModernBuilderWindow::SaveFileDialog(const wchar_t *filter, const wchar_t *title, const wchar_t *defaultName)
    {
        wchar_t filename[MAX_PATH] = {};
        if (defaultName)
            wcscpy_s(filename, defaultName);

        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_hwnd;
        ofn.lpstrFilter = filter;
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = title;
        ofn.Flags = OFN_OVERWRITEPROMPT;

        if (GetSaveFileNameW(&ofn))
            return filename;
        return L"";
    }

} // namespace autopatch
