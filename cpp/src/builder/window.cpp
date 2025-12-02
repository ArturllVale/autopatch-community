#include "window.h"
#include "../core/utils.h"
#include <CommCtrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <windowsx.h>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Comdlg32.lib")

namespace autopatch
{

    // Constantes de layout
    static constexpr int WINDOW_WIDTH = 800;
    static constexpr int WINDOW_HEIGHT = 600;
    static constexpr int MARGIN = 10;
    static constexpr int LABEL_HEIGHT = 20;
    static constexpr int EDIT_HEIGHT = 24;
    static constexpr int BUTTON_HEIGHT = 28;
    static constexpr int BUTTON_WIDTH = 100;

    BuilderWindow::BuilderWindow() = default;
    BuilderWindow::~BuilderWindow() = default;

    bool BuilderWindow::Create(HINSTANCE hInstance)
    {
        m_hInstance = hInstance;

        // Registra a classe da janela
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = L"AutoPatchBuilderClass";
        wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

        if (!RegisterClassExW(&wc))
        {
            return false;
        }

        // Calcula posição centralizada
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int x = (screenW - WINDOW_WIDTH) / 2;
        int y = (screenH - WINDOW_HEIGHT) / 2;

        // Cria a janela
        m_hwnd = CreateWindowExW(
            0,
            L"AutoPatchBuilderClass",
            L"AutoPatch Builder",
            WS_OVERLAPPEDWINDOW,
            x, y, WINDOW_WIDTH, WINDOW_HEIGHT,
            nullptr, nullptr, hInstance, this);

        if (!m_hwnd)
        {
            return false;
        }

        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);

        return true;
    }

    int BuilderWindow::Run()
    {
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            if (!IsDialogMessage(m_hwnd, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        return static_cast<int>(msg.wParam);
    }

    LRESULT CALLBACK BuilderWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        BuilderWindow *pThis = nullptr;

        if (msg == WM_NCCREATE)
        {
            CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
            pThis = static_cast<BuilderWindow *>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
            pThis->m_hwnd = hwnd;
        }
        else
        {
            pThis = reinterpret_cast<BuilderWindow *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (pThis)
        {
            return pThis->HandleMessage(msg, wParam, lParam);
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT BuilderWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_CREATE:
            InitControls();
            return 0;

        case WM_COMMAND:
            OnCommand(wParam, lParam);
            return 0;

        case WM_NOTIFY:
            OnNotify(reinterpret_cast<NMHDR *>(lParam));
            return 0;

        case WM_SIZE:
        {
            // Redimensiona controles
            RECT rc;
            GetClientRect(m_hwnd, &rc);

            // Tab control
            if (m_tabControl)
            {
                SetWindowPos(m_tabControl, nullptr,
                             MARGIN, MARGIN,
                             rc.right - MARGIN * 2,
                             rc.bottom - 60,
                             SWP_NOZORDER);
            }

            // Status bar
            if (m_statusBar)
            {
                SendMessage(m_statusBar, WM_SIZE, 0, 0);
            }

            // Progress bar
            if (m_progressBar)
            {
                SetWindowPos(m_progressBar, nullptr,
                             rc.right - 210, rc.bottom - 28,
                             200, 20,
                             SWP_NOZORDER);
            }

            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }

    void BuilderWindow::InitControls()
    {
        // Inicializa Common Controls
        INITCOMMONCONTROLSEX icex = {};
        icex.dwSize = sizeof(icex);
        icex.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_BAR_CLASSES;
        InitCommonControlsEx(&icex);

        // Cria Tab Control
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        m_tabControl = CreateWindowExW(
            0, WC_TABCONTROLW, L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            MARGIN, MARGIN,
            rc.right - MARGIN * 2,
            rc.bottom - 60,
            m_hwnd, (HMENU)ID_TAB_CONTROL, m_hInstance, nullptr);

        // Configura fonte
        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        SendMessage(m_tabControl, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Status bar
        m_statusBar = CreateWindowExW(
            0, STATUSCLASSNAMEW, L"Ready",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            m_hwnd, nullptr, m_hInstance, nullptr);

        // Progress bar na status bar
        m_progressBar = CreateWindowExW(
            0, PROGRESS_CLASSW, L"",
            WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            rc.right - 210, rc.bottom - 28,
            200, 20,
            m_hwnd, nullptr, m_hInstance, nullptr);

        // Cria tabs
        CreateTabs();
    }

    void BuilderWindow::CreateTabs()
    {
        // Adiciona tabs
        TCITEMW tie = {};
        tie.mask = TCIF_TEXT;

        tie.pszText = (LPWSTR)L"General";
        TabCtrl_InsertItem(m_tabControl, 0, &tie);

        tie.pszText = (LPWSTR)L"Image Mode";
        TabCtrl_InsertItem(m_tabControl, 1, &tie);

        tie.pszText = (LPWSTR)L"HTML Mode";
        TabCtrl_InsertItem(m_tabControl, 2, &tie);

        tie.pszText = (LPWSTR)L"UI Elements";
        TabCtrl_InsertItem(m_tabControl, 3, &tie);

        tie.pszText = (LPWSTR)L"Build";
        TabCtrl_InsertItem(m_tabControl, 4, &tie);

        // Obtém área do display
        RECT rcTab;
        GetClientRect(m_tabControl, &rcTab);
        TabCtrl_AdjustRect(m_tabControl, FALSE, &rcTab);

        // Cria páginas (containers para cada tab)
        for (int i = 0; i < 5; i++)
        {
            HWND page = CreateWindowExW(
                0, L"STATIC", L"",
                WS_CHILD | (i == 0 ? WS_VISIBLE : 0),
                rcTab.left, rcTab.top,
                rcTab.right - rcTab.left,
                rcTab.bottom - rcTab.top,
                m_tabControl, nullptr, m_hInstance, nullptr);
            m_tabPages.push_back(page);
        }

        // Cria controles em cada página
        CreateGeneralTab();
        CreateImageModeTab();
        CreateHtmlModeTab();
        CreateButtonsTab();
        CreateOutputTab();
    }

    void BuilderWindow::CreateGeneralTab()
    {
        HWND page = m_tabPages[0];
        int y = 10;
        int labelW = 120;
        int editW = 400;

        // Server Name
        CreateLabel(page, L"Server Name:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editServerName = CreateEdit(page, ID_EDIT_SERVER_NAME, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        y += 30;

        // Patch List URL
        CreateLabel(page, L"Patch List URL:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editPatchUrl = CreateEdit(page, ID_EDIT_PATCH_URL, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        y += 30;

        // News URL
        CreateLabel(page, L"News URL:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editNewsUrl = CreateEdit(page, ID_EDIT_NEWS_URL, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        y += 30;

        // Client EXE
        CreateLabel(page, L"Client EXE:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editClientExe = CreateEdit(page, ID_EDIT_CLIENT_EXE, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        SetWindowTextW(m_editClientExe, L"ragexe.exe");
        y += 30;

        // Client Args
        CreateLabel(page, L"Client Args:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editClientArgs = CreateEdit(page, ID_EDIT_CLIENT_ARGS, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        y += 30;

        // GRF Files
        CreateLabel(page, L"GRF Files:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editGrfFiles = CreateEdit(page, ID_EDIT_GRF_FILES, MARGIN + labelW, y, editW, 60, true);
        SetWindowTextW(m_editGrfFiles, L"data.grf");
        y += 70;

        // UI Mode
        CreateLabel(page, L"UI Mode:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_comboUiMode = CreateComboBox(page, ID_COMBO_UI_MODE, MARGIN + labelW, y, 150, 100);
        SendMessageW(m_comboUiMode, CB_ADDSTRING, 0, (LPARAM)L"Image Mode");
        SendMessageW(m_comboUiMode, CB_ADDSTRING, 0, (LPARAM)L"HTML Mode");
        SendMessageW(m_comboUiMode, CB_SETCURSEL, 0, 0);
        y += 30;

        // Window Size
        CreateLabel(page, L"Window Size:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editWidth = CreateEdit(page, ID_EDIT_WIDTH, MARGIN + labelW, y, 60, EDIT_HEIGHT);
        SetWindowTextW(m_editWidth, L"800");
        CreateLabel(page, L"x", MARGIN + labelW + 65, y, 15, LABEL_HEIGHT);
        m_editHeight = CreateEdit(page, ID_EDIT_HEIGHT, MARGIN + labelW + 80, y, 60, EDIT_HEIGHT);
        SetWindowTextW(m_editHeight, L"600");
        y += 30;

        // Checkboxes
        m_checkResize = CreateCheckBox(page, ID_CHECK_RESIZE, L"Allow Resize", MARGIN, y, 150, 20);
        m_checkTaskbar = CreateCheckBox(page, ID_CHECK_TASKBAR, L"Show in Taskbar", MARGIN + 160, y, 150, 20);
        Button_SetCheck(m_checkTaskbar, BST_CHECKED);
    }

    void BuilderWindow::CreateImageModeTab()
    {
        HWND page = m_tabPages[1];
        int y = 10;
        int labelW = 120;
        int editW = 350;
        int btnW = 80;

        // Background Image
        CreateLabel(page, L"Background:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editBackgroundPath = CreateEdit(page, ID_EDIT_BG_PATH, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        m_btnBrowseBackground = CreateButton(page, ID_BTN_BROWSE_BG, L"Browse...", MARGIN + labelW + editW + 5, y, btnW, EDIT_HEIGHT);
        y += 35;

        // Button Normal
        CreateLabel(page, L"Button Normal:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editButtonNormal = CreateEdit(page, ID_EDIT_BTN_NORMAL, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        CreateButton(page, ID_BTN_BROWSE_BTN_NORMAL, L"Browse...", MARGIN + labelW + editW + 5, y, btnW, EDIT_HEIGHT);
        y += 30;

        // Button Hover
        CreateLabel(page, L"Button Hover:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editButtonHover = CreateEdit(page, ID_EDIT_BTN_HOVER, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        CreateButton(page, ID_BTN_BROWSE_BTN_HOVER, L"Browse...", MARGIN + labelW + editW + 5, y, btnW, EDIT_HEIGHT);
        y += 30;

        // Button Pressed
        CreateLabel(page, L"Button Pressed:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editButtonPressed = CreateEdit(page, ID_EDIT_BTN_PRESSED, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        CreateButton(page, ID_BTN_BROWSE_BTN_PRESSED, L"Browse...", MARGIN + labelW + editW + 5, y, btnW, EDIT_HEIGHT);
        y += 35;

        // Custom Font
        CreateLabel(page, L"Custom Font:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editFontPath = CreateEdit(page, ID_EDIT_FONT, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        CreateButton(page, ID_BTN_BROWSE_FONT, L"Browse...", MARGIN + labelW + editW + 5, y, btnW, EDIT_HEIGHT);
    }

    void BuilderWindow::CreateHtmlModeTab()
    {
        HWND page = m_tabPages[2];
        int y = 10;
        int labelW = 120;
        int editW = 350;
        int btnW = 80;

        // HTML File
        CreateLabel(page, L"HTML File:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editHtmlPath = CreateEdit(page, ID_EDIT_HTML, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        CreateButton(page, ID_BTN_BROWSE_HTML, L"Browse...", MARGIN + labelW + editW + 5, y, btnW, EDIT_HEIGHT);
        y += 30;

        // CSS File
        CreateLabel(page, L"CSS File:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editCssPath = CreateEdit(page, ID_EDIT_CSS, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        CreateButton(page, ID_BTN_BROWSE_CSS, L"Browse...", MARGIN + labelW + editW + 5, y, btnW, EDIT_HEIGHT);
        y += 30;

        // JS File
        CreateLabel(page, L"JS File:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editJsPath = CreateEdit(page, ID_EDIT_JS, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        CreateButton(page, ID_BTN_BROWSE_JS, L"Browse...", MARGIN + labelW + editW + 5, y, btnW, EDIT_HEIGHT);
        y += 40;

        // Element IDs
        CreateGroupBox(page, L"HTML Element IDs", MARGIN, y, 550, 150);
        y += 20;

        CreateLabel(page, L"Start Button ID:", MARGIN + 10, y, 110, LABEL_HEIGHT);
        m_editStartBtnId = CreateEdit(page, ID_EDIT_START_BTN_ID, MARGIN + 125, y, 150, EDIT_HEIGHT);
        SetWindowTextW(m_editStartBtnId, L"btn-start");
        y += 28;

        CreateLabel(page, L"Progress Bar ID:", MARGIN + 10, y, 110, LABEL_HEIGHT);
        m_editProgressId = CreateEdit(page, ID_EDIT_PROGRESS_ID, MARGIN + 125, y, 150, EDIT_HEIGHT);
        SetWindowTextW(m_editProgressId, L"progress-bar");
        y += 28;

        CreateLabel(page, L"Status Label ID:", MARGIN + 10, y, 110, LABEL_HEIGHT);
        m_editStatusId = CreateEdit(page, ID_EDIT_STATUS_ID, MARGIN + 125, y, 150, EDIT_HEIGHT);
        SetWindowTextW(m_editStatusId, L"status-text");
        y += 28;

        CreateLabel(page, L"Close Button ID:", MARGIN + 10, y, 110, LABEL_HEIGHT);
        m_editCloseBtnId = CreateEdit(page, ID_EDIT_CLOSE_BTN_ID, MARGIN + 125, y, 150, EDIT_HEIGHT);
        SetWindowTextW(m_editCloseBtnId, L"btn-close");

        CreateLabel(page, L"Minimize ID:", MARGIN + 290, y, 90, LABEL_HEIGHT);
        m_editMinBtnId = CreateEdit(page, ID_EDIT_MIN_BTN_ID, MARGIN + 385, y, 150, EDIT_HEIGHT);
        SetWindowTextW(m_editMinBtnId, L"btn-minimize");
    }

    void BuilderWindow::CreateButtonsTab()
    {
        HWND page = m_tabPages[3];
        int y = 10;

        // Buttons list
        CreateLabel(page, L"Buttons:", MARGIN, y, 100, LABEL_HEIGHT);
        y += 20;

        m_listButtons = CreateListView(page, ID_LIST_BUTTONS, MARGIN, y, 350, 120);

        // Configura colunas do ListView
        LVCOLUMNW col = {};
        col.mask = LVCF_TEXT | LVCF_WIDTH;

        col.pszText = (LPWSTR)L"Action";
        col.cx = 100;
        ListView_InsertColumn(m_listButtons, 0, &col);

        col.pszText = (LPWSTR)L"X";
        col.cx = 50;
        ListView_InsertColumn(m_listButtons, 1, &col);

        col.pszText = (LPWSTR)L"Y";
        col.cx = 50;
        ListView_InsertColumn(m_listButtons, 2, &col);

        col.pszText = (LPWSTR)L"Width";
        col.cx = 60;
        ListView_InsertColumn(m_listButtons, 3, &col);

        col.pszText = (LPWSTR)L"Height";
        col.cx = 60;
        ListView_InsertColumn(m_listButtons, 4, &col);

        CreateButton(page, ID_BTN_ADD_BUTTON, L"Add", MARGIN + 360, y, 70, BUTTON_HEIGHT);
        CreateButton(page, ID_BTN_REMOVE_BUTTON, L"Remove", MARGIN + 360, y + 35, 70, BUTTON_HEIGHT);

        y += 135;

        // Labels list
        CreateLabel(page, L"Labels:", MARGIN, y, 100, LABEL_HEIGHT);
        y += 20;

        m_listLabels = CreateListView(page, ID_LIST_LABELS, MARGIN, y, 350, 100);

        col.pszText = (LPWSTR)L"ID";
        col.cx = 80;
        ListView_InsertColumn(m_listLabels, 0, &col);

        col.pszText = (LPWSTR)L"Text";
        col.cx = 100;
        ListView_InsertColumn(m_listLabels, 1, &col);

        col.pszText = (LPWSTR)L"X";
        col.cx = 50;
        ListView_InsertColumn(m_listLabels, 2, &col);

        col.pszText = (LPWSTR)L"Y";
        col.cx = 50;
        ListView_InsertColumn(m_listLabels, 3, &col);

        CreateButton(page, ID_BTN_ADD_LABEL, L"Add", MARGIN + 360, y, 70, BUTTON_HEIGHT);
        CreateButton(page, ID_BTN_REMOVE_LABEL, L"Remove", MARGIN + 360, y + 35, 70, BUTTON_HEIGHT);

        y += 115;

        // Progress Bar config
        CreateGroupBox(page, L"Progress Bar", MARGIN, y, 300, 80);
        y += 25;

        CreateLabel(page, L"X:", MARGIN + 10, y, 20, LABEL_HEIGHT);
        m_editProgressX = CreateEdit(page, ID_EDIT_PB_X, MARGIN + 30, y, 50, EDIT_HEIGHT);

        CreateLabel(page, L"Y:", MARGIN + 90, y, 20, LABEL_HEIGHT);
        m_editProgressY = CreateEdit(page, ID_EDIT_PB_Y, MARGIN + 110, y, 50, EDIT_HEIGHT);

        CreateLabel(page, L"W:", MARGIN + 170, y, 20, LABEL_HEIGHT);
        m_editProgressW = CreateEdit(page, ID_EDIT_PB_W, MARGIN + 195, y, 50, EDIT_HEIGHT);

        CreateLabel(page, L"H:", MARGIN + 250, y, 20, LABEL_HEIGHT);
        m_editProgressH = CreateEdit(page, ID_EDIT_PB_H, MARGIN + 270, y, 50, EDIT_HEIGHT);
    }

    void BuilderWindow::CreateOutputTab()
    {
        HWND page = m_tabPages[4];
        int y = 10;
        int labelW = 120;
        int editW = 350;
        int btnW = 80;

        // Template Path
        CreateLabel(page, L"Template EXE:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editTemplatePath = CreateEdit(page, ID_EDIT_TEMPLATE, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        CreateButton(page, ID_BTN_BROWSE_TEMPLATE, L"Browse...", MARGIN + labelW + editW + 5, y, btnW, EDIT_HEIGHT);
        y += 35;

        // Output Path
        CreateLabel(page, L"Output Path:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editOutputPath = CreateEdit(page, ID_EDIT_OUTPUT, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        CreateButton(page, ID_BTN_BROWSE_OUTPUT, L"Browse...", MARGIN + labelW + editW + 5, y, btnW, EDIT_HEIGHT);
        y += 35;

        // Icon
        CreateLabel(page, L"Custom Icon:", MARGIN, y, labelW, LABEL_HEIGHT);
        m_editIconPath = CreateEdit(page, ID_EDIT_ICON, MARGIN + labelW, y, editW, EDIT_HEIGHT);
        CreateButton(page, ID_BTN_BROWSE_ICON, L"Browse...", MARGIN + labelW + editW + 5, y, btnW, EDIT_HEIGHT);
        y += 45;

        // Build buttons
        m_btnBuild = CreateButton(page, ID_BTN_BUILD, L"Build Patcher", MARGIN, y, 150, 35);
        m_btnPreview = CreateButton(page, ID_BTN_PREVIEW, L"Preview", MARGIN + 160, y, 100, 35);
        y += 50;

        // Log
        CreateLabel(page, L"Build Log:", MARGIN, y, 100, LABEL_HEIGHT);
        y += 20;

        m_editLog = CreateEdit(page, ID_EDIT_LOG, MARGIN, y, 550, 150, true);
        SendMessageW(m_editLog, EM_SETREADONLY, TRUE, 0);
    }

    void BuilderWindow::OnCommand(WPARAM wParam, LPARAM lParam)
    {
        int id = LOWORD(wParam);

        switch (id)
        {
        case ID_BTN_BROWSE_BG:
            OnBrowseBackground();
            break;
        case ID_BTN_BROWSE_BTN_NORMAL:
            OnBrowseButtonNormal();
            break;
        case ID_BTN_BROWSE_BTN_HOVER:
            OnBrowseButtonHover();
            break;
        case ID_BTN_BROWSE_BTN_PRESSED:
            OnBrowseButtonPressed();
            break;
        case ID_BTN_BROWSE_ICON:
            OnBrowseIcon();
            break;
        case ID_BTN_BROWSE_FONT:
            OnBrowseFont();
            break;
        case ID_BTN_BROWSE_HTML:
            OnBrowseHtml();
            break;
        case ID_BTN_BROWSE_CSS:
            OnBrowseCss();
            break;
        case ID_BTN_BROWSE_JS:
            OnBrowseJs();
            break;
        case ID_BTN_BROWSE_OUTPUT:
            OnBrowseOutput();
            break;
        case ID_BTN_BROWSE_TEMPLATE:
            OnBrowseTemplate();
            break;
        case ID_BTN_BUILD:
            OnBuild();
            break;
        case ID_BTN_PREVIEW:
            OnPreview();
            break;
        case ID_BTN_ADD_BUTTON:
            OnAddButton();
            break;
        case ID_BTN_REMOVE_BUTTON:
            OnRemoveButton();
            break;
        case ID_BTN_ADD_LABEL:
            OnAddLabel();
            break;
        case ID_BTN_REMOVE_LABEL:
            OnRemoveLabel();
            break;
        }
    }

    void BuilderWindow::OnNotify(NMHDR *pnmh)
    {
        if (pnmh->idFrom == ID_TAB_CONTROL && pnmh->code == TCN_SELCHANGE)
        {
            OnTabChanged();
        }
    }

    void BuilderWindow::OnTabChanged()
    {
        int sel = TabCtrl_GetCurSel(m_tabControl);

        // Esconde todas as páginas
        for (int i = 0; i < m_tabPages.size(); i++)
        {
            ShowWindow(m_tabPages[i], i == sel ? SW_SHOW : SW_HIDE);
        }

        m_currentTab = sel;
    }

    // Browse handlers
    void BuilderWindow::OnBrowseBackground()
    {
        std::wstring path = OpenFileDialog(
            L"Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0All Files\0*.*\0",
            L"Select Background Image");
        if (!path.empty())
        {
            SetWindowTextW(m_editBackgroundPath, path.c_str());
        }
    }

    void BuilderWindow::OnBrowseButtonNormal()
    {
        std::wstring path = OpenFileDialog(
            L"Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0All Files\0*.*\0",
            L"Select Button Normal Image");
        if (!path.empty())
        {
            SetWindowTextW(m_editButtonNormal, path.c_str());
        }
    }

    void BuilderWindow::OnBrowseButtonHover()
    {
        std::wstring path = OpenFileDialog(
            L"Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0All Files\0*.*\0",
            L"Select Button Hover Image");
        if (!path.empty())
        {
            SetWindowTextW(m_editButtonHover, path.c_str());
        }
    }

    void BuilderWindow::OnBrowseButtonPressed()
    {
        std::wstring path = OpenFileDialog(
            L"Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0All Files\0*.*\0",
            L"Select Button Pressed Image");
        if (!path.empty())
        {
            SetWindowTextW(m_editButtonPressed, path.c_str());
        }
    }

    void BuilderWindow::OnBrowseIcon()
    {
        std::wstring path = OpenFileDialog(
            L"Icon Files\0*.ico\0All Files\0*.*\0",
            L"Select Icon");
        if (!path.empty())
        {
            SetWindowTextW(m_editIconPath, path.c_str());
        }
    }

    void BuilderWindow::OnBrowseFont()
    {
        std::wstring path = OpenFileDialog(
            L"Font Files\0*.ttf;*.otf\0All Files\0*.*\0",
            L"Select Font");
        if (!path.empty())
        {
            SetWindowTextW(m_editFontPath, path.c_str());
        }
    }

    void BuilderWindow::OnBrowseHtml()
    {
        std::wstring path = OpenFileDialog(
            L"HTML Files\0*.html;*.htm\0All Files\0*.*\0",
            L"Select HTML File");
        if (!path.empty())
        {
            SetWindowTextW(m_editHtmlPath, path.c_str());
        }
    }

    void BuilderWindow::OnBrowseCss()
    {
        std::wstring path = OpenFileDialog(
            L"CSS Files\0*.css\0All Files\0*.*\0",
            L"Select CSS File");
        if (!path.empty())
        {
            SetWindowTextW(m_editCssPath, path.c_str());
        }
    }

    void BuilderWindow::OnBrowseJs()
    {
        std::wstring path = OpenFileDialog(
            L"JavaScript Files\0*.js\0All Files\0*.*\0",
            L"Select JavaScript File");
        if (!path.empty())
        {
            SetWindowTextW(m_editJsPath, path.c_str());
        }
    }

    void BuilderWindow::OnBrowseTemplate()
    {
        std::wstring path = OpenFileDialog(
            L"Executable Files\0*.exe\0All Files\0*.*\0",
            L"Select Template EXE");
        if (!path.empty())
        {
            SetWindowTextW(m_editTemplatePath, path.c_str());
        }
    }

    void BuilderWindow::OnBrowseOutput()
    {
        std::wstring path = SaveFileDialog(
            L"Executable Files\0*.exe\0All Files\0*.*\0",
            L"Save Patcher As",
            L"exe");
        if (!path.empty())
        {
            SetWindowTextW(m_editOutputPath, path.c_str());
        }
    }

    void BuilderWindow::OnBuild()
    {
        // Atualiza config do UI
        UpdateConfigFromUI();

        // Obtém caminhos
        wchar_t templatePath[MAX_PATH], outputPath[MAX_PATH];
        GetWindowTextW(m_editTemplatePath, templatePath, MAX_PATH);
        GetWindowTextW(m_editOutputPath, outputPath, MAX_PATH);

        if (wcslen(templatePath) == 0)
        {
            ShowError(L"Please select the template EXE file.");
            return;
        }

        if (wcslen(outputPath) == 0)
        {
            ShowError(L"Please select the output path.");
            return;
        }

        // Configura embedder
        m_embedder.SetClientTemplatePath(templatePath);

        // Carrega imagens se Image mode
        int uiMode = static_cast<int>(SendMessageW(m_comboUiMode, CB_GETCURSEL, 0, 0));

        if (uiMode == 0)
        {
            wchar_t bgPath[MAX_PATH];
            GetWindowTextW(m_editBackgroundPath, bgPath, MAX_PATH);
            if (wcslen(bgPath) > 0)
            {
                m_embedder.LoadBackgroundImage(bgPath);
            }

            wchar_t btnNormal[MAX_PATH], btnHover[MAX_PATH], btnPressed[MAX_PATH];
            GetWindowTextW(m_editButtonNormal, btnNormal, MAX_PATH);
            GetWindowTextW(m_editButtonHover, btnHover, MAX_PATH);
            GetWindowTextW(m_editButtonPressed, btnPressed, MAX_PATH);
            m_embedder.LoadButtonImages(btnNormal, btnHover, btnPressed);

            wchar_t fontPath[MAX_PATH];
            GetWindowTextW(m_editFontPath, fontPath, MAX_PATH);
            if (wcslen(fontPath) > 0)
            {
                m_embedder.LoadFont(fontPath);
            }
        }
        else
        {
            wchar_t htmlPath[MAX_PATH], cssPath[MAX_PATH], jsPath[MAX_PATH];
            GetWindowTextW(m_editHtmlPath, htmlPath, MAX_PATH);
            GetWindowTextW(m_editCssPath, cssPath, MAX_PATH);
            GetWindowTextW(m_editJsPath, jsPath, MAX_PATH);
            m_embedder.LoadHtmlFiles(htmlPath, cssPath, jsPath);
        }

        // Carrega ícone
        wchar_t iconPath[MAX_PATH];
        GetWindowTextW(m_editIconPath, iconPath, MAX_PATH);
        if (wcslen(iconPath) > 0)
        {
            m_embedder.LoadIcon(iconPath);
        }

        // Limpa log
        SetWindowTextW(m_editLog, L"");

        // Build
        auto callback = [this](int percent, const std::wstring &status)
        {
            UpdateProgress(percent);
            SetStatus(status);

            // Adiciona ao log
            wchar_t log[512];
            swprintf(log, 512, L"[%d%%] %s\r\n", percent, status.c_str());

            int len = GetWindowTextLengthW(m_editLog);
            SendMessageW(m_editLog, EM_SETSEL, len, len);
            SendMessageW(m_editLog, EM_REPLACESEL, FALSE, (LPARAM)log);
        };

        callback(0, L"Starting build...");

        BuildResult result = m_embedder.Build(outputPath, callback);

        if (result.success)
        {
            wchar_t msg[256];
            swprintf(msg, 256, L"Build successful!\nOutput: %s\nSize: %llu bytes",
                     result.outputPath.c_str(), result.outputSize);
            ShowInfo(msg);
        }
        else
        {
            ShowError(result.errorMessage);
        }
    }

    void BuilderWindow::OnPreview()
    {
        ShowInfo(L"Preview feature not yet implemented.\nPlease build the patcher and test it directly.");
    }

    void BuilderWindow::OnAddButton()
    {
        // Abre diálogo para adicionar botão
        // TODO: Implementar diálogo de edição
        ShowInfo(L"Button editor not yet implemented.\nEdit buttons in code for now.");
    }

    void BuilderWindow::OnRemoveButton()
    {
        int sel = ListView_GetNextItem(m_listButtons, -1, LVNI_SELECTED);
        if (sel >= 0)
        {
            ListView_DeleteItem(m_listButtons, sel);
        }
    }

    void BuilderWindow::OnAddLabel()
    {
        ShowInfo(L"Label editor not yet implemented.\nEdit labels in code for now.");
    }

    void BuilderWindow::OnRemoveLabel()
    {
        int sel = ListView_GetNextItem(m_listLabels, -1, LVNI_SELECTED);
        if (sel >= 0)
        {
            ListView_DeleteItem(m_listLabels, sel);
        }
    }

    void BuilderWindow::UpdateConfigFromUI()
    {
        PatcherConfig config;

        wchar_t buffer[1024];

        GetWindowTextW(m_editServerName, buffer, 1024);
        config.serverName = utils::WideToUtf8(buffer);

        GetWindowTextW(m_editPatchUrl, buffer, 1024);
        config.patchListUrl = utils::WideToUtf8(buffer);

        GetWindowTextW(m_editNewsUrl, buffer, 1024);
        config.newsUrl = utils::WideToUtf8(buffer);

        GetWindowTextW(m_editClientExe, buffer, 1024);
        config.clientExe = utils::WideToUtf8(buffer);

        GetWindowTextW(m_editClientArgs, buffer, 1024);
        config.clientArgs = utils::WideToUtf8(buffer);

        // GRF files (um por linha)
        GetWindowTextW(m_editGrfFiles, buffer, 1024);
        std::wstring grfList = buffer;
        config.grfFiles.clear();

        size_t pos = 0;
        while (pos < grfList.size())
        {
            size_t end = grfList.find(L'\n', pos);
            if (end == std::wstring::npos)
                end = grfList.size();

            std::wstring line = grfList.substr(pos, end - pos);
            // Remove \r
            if (!line.empty() && line.back() == L'\r')
            {
                line.pop_back();
            }
            if (!line.empty())
            {
                config.grfFiles.push_back(utils::WideToUtf8(line));
            }
            pos = end + 1;
        }

        // UI Mode
        int uiMode = static_cast<int>(SendMessageW(m_comboUiMode, CB_GETCURSEL, 0, 0));
        config.uiType = uiMode == 0 ? UIType::Image : UIType::Html;

        // Window size
        GetWindowTextW(m_editWidth, buffer, 32);
        config.windowWidth = _wtoi(buffer);

        GetWindowTextW(m_editHeight, buffer, 32);
        config.windowHeight = _wtoi(buffer);

        config.allowResize = Button_GetCheck(m_checkResize) == BST_CHECKED;
        config.showInTaskbar = Button_GetCheck(m_checkTaskbar) == BST_CHECKED;

        // HTML mode config
        if (config.uiType == UIType::Html)
        {
            config.htmlMode = std::make_shared<HtmlModeConfig>();

            GetWindowTextW(m_editStartBtnId, buffer, 256);
            config.htmlMode->startButtonId = utils::WideToUtf8(buffer);

            GetWindowTextW(m_editProgressId, buffer, 256);
            config.htmlMode->progressBarId = utils::WideToUtf8(buffer);

            GetWindowTextW(m_editStatusId, buffer, 256);
            config.htmlMode->statusLabelId = utils::WideToUtf8(buffer);

            GetWindowTextW(m_editCloseBtnId, buffer, 256);
            config.htmlMode->closeButtonId = utils::WideToUtf8(buffer);

            GetWindowTextW(m_editMinBtnId, buffer, 256);
            config.htmlMode->minimizeButtonId = utils::WideToUtf8(buffer);
        }
        else
        {
            config.imageMode = std::make_shared<ImageModeConfig>();
            // TODO: Preencher com botões/labels da lista
        }

        m_embedder.SetConfig(config);
    }

    void BuilderWindow::UpdateUIFromConfig()
    {
        // TODO: Implementar atualização do UI a partir da config
    }

    std::wstring BuilderWindow::OpenFileDialog(const wchar_t *filter, const wchar_t *title)
    {
        wchar_t filename[MAX_PATH] = L"";

        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_hwnd;
        ofn.lpstrFilter = filter;
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = title;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        if (GetOpenFileNameW(&ofn))
        {
            return filename;
        }
        return L"";
    }

    std::wstring BuilderWindow::SaveFileDialog(const wchar_t *filter, const wchar_t *title, const wchar_t *defaultExt)
    {
        wchar_t filename[MAX_PATH] = L"";

        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_hwnd;
        ofn.lpstrFilter = filter;
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = title;
        ofn.lpstrDefExt = defaultExt;
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

        if (GetSaveFileNameW(&ofn))
        {
            return filename;
        }
        return L"";
    }

    void BuilderWindow::ShowError(const std::wstring &message)
    {
        MessageBoxW(m_hwnd, message.c_str(), L"Error", MB_ICONERROR | MB_OK);
    }

    void BuilderWindow::ShowInfo(const std::wstring &message)
    {
        MessageBoxW(m_hwnd, message.c_str(), L"Information", MB_ICONINFORMATION | MB_OK);
    }

    void BuilderWindow::SetStatus(const std::wstring &status)
    {
        SendMessageW(m_statusBar, SB_SETTEXTW, 0, (LPARAM)status.c_str());
    }

    void BuilderWindow::UpdateProgress(int percent)
    {
        SendMessageW(m_progressBar, PBM_SETPOS, percent, 0);
    }

    // Control creation helpers
    HWND BuilderWindow::CreateLabel(HWND parent, const wchar_t *text, int x, int y, int w, int h)
    {
        HWND hwnd = CreateWindowExW(
            0, L"STATIC", text,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, y, w, h,
            parent, nullptr, m_hInstance, nullptr);
        SendMessage(hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        return hwnd;
    }

    HWND BuilderWindow::CreateEdit(HWND parent, int id, int x, int y, int w, int h, bool multiline)
    {
        DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
        if (multiline)
        {
            style |= ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL;
        }

        HWND hwnd = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            style,
            x, y, w, h,
            parent, (HMENU)(INT_PTR)id, m_hInstance, nullptr);
        SendMessage(hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        return hwnd;
    }

    HWND BuilderWindow::CreateButton(HWND parent, int id, const wchar_t *text, int x, int y, int w, int h)
    {
        HWND hwnd = CreateWindowExW(
            0, L"BUTTON", text,
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            x, y, w, h,
            parent, (HMENU)(INT_PTR)id, m_hInstance, nullptr);
        SendMessage(hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        return hwnd;
    }

    HWND BuilderWindow::CreateCheckBox(HWND parent, int id, const wchar_t *text, int x, int y, int w, int h)
    {
        HWND hwnd = CreateWindowExW(
            0, L"BUTTON", text,
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            x, y, w, h,
            parent, (HMENU)(INT_PTR)id, m_hInstance, nullptr);
        SendMessage(hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        return hwnd;
    }

    HWND BuilderWindow::CreateComboBox(HWND parent, int id, int x, int y, int w, int h)
    {
        HWND hwnd = CreateWindowExW(
            0, WC_COMBOBOXW, L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            x, y, w, h,
            parent, (HMENU)(INT_PTR)id, m_hInstance, nullptr);
        SendMessage(hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        return hwnd;
    }

    HWND BuilderWindow::CreateListView(HWND parent, int id, int x, int y, int w, int h)
    {
        HWND hwnd = CreateWindowExW(
            WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            x, y, w, h,
            parent, (HMENU)(INT_PTR)id, m_hInstance, nullptr);
        ListView_SetExtendedListViewStyle(hwnd, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        return hwnd;
    }

    HWND BuilderWindow::CreateSpinner(HWND parent, int id, int x, int y, int w, int h, int minVal, int maxVal)
    {
        // Cria edit buddy
        HWND edit = CreateEdit(parent, id, x, y, w - 20, h);

        // Cria updown control
        HWND updown = CreateWindowExW(
            0, UPDOWN_CLASSW, L"",
            WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_SETBUDDYINT | UDS_ARROWKEYS,
            0, 0, 0, 0,
            parent, nullptr, m_hInstance, nullptr);

        SendMessage(updown, UDM_SETBUDDY, (WPARAM)edit, 0);
        SendMessage(updown, UDM_SETRANGE32, minVal, maxVal);

        return edit;
    }

    HWND BuilderWindow::CreateGroupBox(HWND parent, const wchar_t *text, int x, int y, int w, int h)
    {
        HWND hwnd = CreateWindowExW(
            0, L"BUTTON", text,
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            x, y, w, h,
            parent, nullptr, m_hInstance, nullptr);
        SendMessage(hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        return hwnd;
    }

} // namespace autopatch
