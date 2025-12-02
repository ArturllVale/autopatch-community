#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "embedder.h"

namespace autopatch
{

    // Forward declarations
    class BuilderControls;

    // Tipos de callback
    using FileSelectedCallback = std::function<void(const std::wstring &path)>;
    using BuildCallback = std::function<void(bool success, const std::wstring &message)>;

    // Classe principal da janela do Builder
    class BuilderWindow
    {
    public:
        BuilderWindow();
        ~BuilderWindow();

        // Inicializa e mostra a janela
        bool Create(HINSTANCE hInstance);
        int Run();

        // Acesso ao embedder
        Embedder &GetEmbedder() { return m_embedder; }

    private:
        // Window Procedure
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        // Inicialização
        void InitControls();
        void CreateTabs();
        void CreateGeneralTab();
        void CreateImageModeTab();
        void CreateHtmlModeTab();
        void CreateButtonsTab();
        void CreateOutputTab();

        // Eventos
        void OnCommand(WPARAM wParam, LPARAM lParam);
        void OnNotify(NMHDR *pnmh);
        void OnTabChanged();
        void OnBrowseBackground();
        void OnBrowseButtonNormal();
        void OnBrowseButtonHover();
        void OnBrowseButtonPressed();
        void OnBrowseIcon();
        void OnBrowseFont();
        void OnBrowseHtml();
        void OnBrowseCss();
        void OnBrowseJs();
        void OnBrowseOutput();
        void OnBrowseTemplate();
        void OnBuild();
        void OnPreview();
        void OnAddButton();
        void OnRemoveButton();
        void OnAddLabel();
        void OnRemoveLabel();

        // Helpers
        std::wstring OpenFileDialog(const wchar_t *filter, const wchar_t *title);
        std::wstring SaveFileDialog(const wchar_t *filter, const wchar_t *title, const wchar_t *defaultExt);
        void UpdateConfigFromUI();
        void UpdateUIFromConfig();
        void ShowError(const std::wstring &message);
        void ShowInfo(const std::wstring &message);
        void SetStatus(const std::wstring &status);
        void UpdateProgress(int percent);

        // Controles
        HWND CreateLabel(HWND parent, const wchar_t *text, int x, int y, int w, int h);
        HWND CreateEdit(HWND parent, int id, int x, int y, int w, int h, bool multiline = false);
        HWND CreateButton(HWND parent, int id, const wchar_t *text, int x, int y, int w, int h);
        HWND CreateCheckBox(HWND parent, int id, const wchar_t *text, int x, int y, int w, int h);
        HWND CreateComboBox(HWND parent, int id, int x, int y, int w, int h);
        HWND CreateListView(HWND parent, int id, int x, int y, int w, int h);
        HWND CreateSpinner(HWND parent, int id, int x, int y, int w, int h, int minVal, int maxVal);
        HWND CreateGroupBox(HWND parent, const wchar_t *text, int x, int y, int w, int h);

    private:
        HWND m_hwnd = nullptr;
        HWND m_tabControl = nullptr;
        HWND m_statusBar = nullptr;
        HWND m_progressBar = nullptr;

        // Tab pages (containers)
        std::vector<HWND> m_tabPages;
        int m_currentTab = 0;

        // Controles por tab
        // General Tab
        HWND m_editServerName = nullptr;
        HWND m_editPatchUrl = nullptr;
        HWND m_editNewsUrl = nullptr;
        HWND m_editClientExe = nullptr;
        HWND m_editClientArgs = nullptr;
        HWND m_editGrfFiles = nullptr;
        HWND m_comboUiMode = nullptr;
        HWND m_editWidth = nullptr;
        HWND m_editHeight = nullptr;
        HWND m_checkResize = nullptr;
        HWND m_checkTaskbar = nullptr;

        // Image Mode Tab
        HWND m_editBackgroundPath = nullptr;
        HWND m_btnBrowseBackground = nullptr;
        HWND m_editButtonNormal = nullptr;
        HWND m_editButtonHover = nullptr;
        HWND m_editButtonPressed = nullptr;
        HWND m_editFontPath = nullptr;

        // HTML Mode Tab
        HWND m_editHtmlPath = nullptr;
        HWND m_editCssPath = nullptr;
        HWND m_editJsPath = nullptr;
        HWND m_editStartBtnId = nullptr;
        HWND m_editProgressId = nullptr;
        HWND m_editStatusId = nullptr;
        HWND m_editCloseBtnId = nullptr;
        HWND m_editMinBtnId = nullptr;

        // Buttons Tab
        HWND m_listButtons = nullptr;
        HWND m_listLabels = nullptr;
        HWND m_editProgressX = nullptr;
        HWND m_editProgressY = nullptr;
        HWND m_editProgressW = nullptr;
        HWND m_editProgressH = nullptr;

        // Output Tab
        HWND m_editTemplatePath = nullptr;
        HWND m_editOutputPath = nullptr;
        HWND m_editIconPath = nullptr;
        HWND m_btnBuild = nullptr;
        HWND m_btnPreview = nullptr;
        HWND m_editLog = nullptr;

        Embedder m_embedder;
        HINSTANCE m_hInstance = nullptr;

        // IDs dos controles
        static constexpr int ID_TAB_CONTROL = 100;
        static constexpr int ID_EDIT_SERVER_NAME = 101;
        static constexpr int ID_EDIT_PATCH_URL = 102;
        static constexpr int ID_EDIT_NEWS_URL = 103;
        static constexpr int ID_EDIT_CLIENT_EXE = 104;
        static constexpr int ID_EDIT_CLIENT_ARGS = 105;
        static constexpr int ID_EDIT_GRF_FILES = 106;
        static constexpr int ID_COMBO_UI_MODE = 107;
        static constexpr int ID_EDIT_WIDTH = 108;
        static constexpr int ID_EDIT_HEIGHT = 109;
        static constexpr int ID_CHECK_RESIZE = 110;
        static constexpr int ID_CHECK_TASKBAR = 111;

        static constexpr int ID_EDIT_BG_PATH = 120;
        static constexpr int ID_BTN_BROWSE_BG = 121;
        static constexpr int ID_EDIT_BTN_NORMAL = 122;
        static constexpr int ID_BTN_BROWSE_BTN_NORMAL = 123;
        static constexpr int ID_EDIT_BTN_HOVER = 124;
        static constexpr int ID_BTN_BROWSE_BTN_HOVER = 125;
        static constexpr int ID_EDIT_BTN_PRESSED = 126;
        static constexpr int ID_BTN_BROWSE_BTN_PRESSED = 127;
        static constexpr int ID_EDIT_FONT = 128;
        static constexpr int ID_BTN_BROWSE_FONT = 129;

        static constexpr int ID_EDIT_HTML = 140;
        static constexpr int ID_BTN_BROWSE_HTML = 141;
        static constexpr int ID_EDIT_CSS = 142;
        static constexpr int ID_BTN_BROWSE_CSS = 143;
        static constexpr int ID_EDIT_JS = 144;
        static constexpr int ID_BTN_BROWSE_JS = 145;
        static constexpr int ID_EDIT_START_BTN_ID = 146;
        static constexpr int ID_EDIT_PROGRESS_ID = 147;
        static constexpr int ID_EDIT_STATUS_ID = 148;
        static constexpr int ID_EDIT_CLOSE_BTN_ID = 149;
        static constexpr int ID_EDIT_MIN_BTN_ID = 150;

        static constexpr int ID_LIST_BUTTONS = 160;
        static constexpr int ID_BTN_ADD_BUTTON = 161;
        static constexpr int ID_BTN_REMOVE_BUTTON = 162;
        static constexpr int ID_LIST_LABELS = 163;
        static constexpr int ID_BTN_ADD_LABEL = 164;
        static constexpr int ID_BTN_REMOVE_LABEL = 165;
        static constexpr int ID_EDIT_PB_X = 166;
        static constexpr int ID_EDIT_PB_Y = 167;
        static constexpr int ID_EDIT_PB_W = 168;
        static constexpr int ID_EDIT_PB_H = 169;

        static constexpr int ID_EDIT_TEMPLATE = 180;
        static constexpr int ID_BTN_BROWSE_TEMPLATE = 181;
        static constexpr int ID_EDIT_OUTPUT = 182;
        static constexpr int ID_BTN_BROWSE_OUTPUT = 183;
        static constexpr int ID_EDIT_ICON = 184;
        static constexpr int ID_BTN_BROWSE_ICON = 185;
        static constexpr int ID_BTN_BUILD = 186;
        static constexpr int ID_BTN_PREVIEW = 187;
        static constexpr int ID_EDIT_LOG = 188;
    };

} // namespace autopatch
