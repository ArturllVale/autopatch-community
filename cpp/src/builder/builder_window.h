#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "modern_ui.h"
#include "embedder.h"

namespace autopatch
{
    // IDs de controles
    enum ControlIDs
    {
        ID_EDIT_SERVER_NAME = 1001,
        ID_EDIT_BASE_URL,
        ID_EDIT_PATCHLIST,
        ID_EDIT_PATCHES_FOLDER,
        ID_EDIT_MAIN_GRF,
        ID_EDIT_GAME_EXE,
        ID_EDIT_GAME_ARGS,
        ID_EDIT_WINDOW_WIDTH,
        ID_EDIT_WINDOW_HEIGHT,
        ID_BTN_SELECT_BACKGROUND,
        ID_BTN_ADD_BUTTON,
        ID_BTN_ADD_LABEL,
        ID_BTN_ADD_PROGRESS,
        ID_BTN_REMOVE_ELEMENT,
        ID_BTN_PREVIEW,
        ID_BTN_GENERATE,
        ID_BTN_OPEN_PROJECT,
        ID_BTN_SAVE_PROJECT,
        ID_LIST_ELEMENTS,
        // Propriedades do elemento
        ID_EDIT_ELEM_X,
        ID_EDIT_ELEM_Y,
        ID_EDIT_ELEM_W,
        ID_EDIT_ELEM_H,
        ID_EDIT_ELEM_TEXT,
        ID_COMBO_ELEM_ACTION,
    };

    class ModernBuilderWindow
    {
    public:
        ModernBuilderWindow();
        ~ModernBuilderWindow();

        bool Create(HINSTANCE hInstance);
        int Run();

    private:
        // Window proc
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        // Inicialização
        void InitGdiPlus();
        void ShutdownGdiPlus();
        void CreateUI();
        void CreateSidebar();
        void CreateMainArea();
        void CreateFooter();
        void CreateEditControls();

        // Desenho
        void OnPaint();
        void PaintSidebar(HDC hdc);
        void PaintMainArea(HDC hdc);
        void PaintFooter(HDC hdc);
        void PaintPropertiesPanel(HDC hdc);

        // Eventos
        void OnCommand(WPARAM wParam, LPARAM lParam);
        void OnMouseMove(int x, int y);
        void OnMouseDown(int x, int y);
        void OnMouseUp(int x, int y);
        void OnSize(int width, int height);

        // Ações
        void SelectImageMode();
        void SelectHtmlMode();
        void SelectBackgroundImage();
        void AddElement(int type);
        void RemoveSelectedElement();
        void UpdatePropertiesPanel();
        void ApplyPropertyChanges();
        void OpenProject();
        void SaveProject();
        void Preview();
        void GenerateExe();
        void ShowExportDialog();

        // Embedder
        bool EmbedConfigInExe(const std::wstring &templatePath, const std::wstring &outputPath);
        std::string GenerateConfigJson();

        // Helpers
        void SetStatus(const std::wstring &text);
        void Invalidate();
        std::wstring OpenFileDialog(const wchar_t *filter, const wchar_t *title);
        std::wstring SaveFileDialog(const wchar_t *filter, const wchar_t *title, const wchar_t *defaultName);

    private:
        HWND m_hwnd = nullptr;
        HINSTANCE m_hInstance = nullptr;
        ULONG_PTR m_gdiplusToken = 0;

        // Layout
        static constexpr int SIDEBAR_WIDTH = 260;
        static constexpr int FOOTER_HEIGHT = 60;
        static constexpr int PROPERTIES_WIDTH = 280;
        int m_width = 1400;
        int m_height = 850;

        // Projeto
        PatcherProjectData m_project;
        std::wstring m_projectPath;

        // UI Controls
        std::unique_ptr<ModeCard> m_imageModeCard;
        std::unique_ptr<ModeCard> m_htmlModeCard;
        std::unique_ptr<DesignCanvas> m_canvas;
        std::unique_ptr<ModernButton> m_btnPreview;
        std::unique_ptr<ModernButton> m_btnGenerate;
        std::unique_ptr<ModernButton> m_btnOpenProject;
        std::unique_ptr<ModernButton> m_btnSaveProject;
        std::unique_ptr<ModernButton> m_btnSelectBg;
        std::unique_ptr<ModernButton> m_btnAddButton;
        std::unique_ptr<ModernButton> m_btnAddLabel;
        std::unique_ptr<ModernButton> m_btnAddProgress;
        std::unique_ptr<ModernButton> m_btnAddProgressBar;
        std::unique_ptr<ModernButton> m_btnRemoveElement;
        std::unique_ptr<ModernCheckBox> m_chkCloseAfterStart;

        // Edit controls (HWND nativos para entrada de texto)
        HWND m_editServerName = nullptr;
        HWND m_editBaseUrl = nullptr;
        HWND m_editPatchlist = nullptr;
        HWND m_editPatchesFolder = nullptr;
        HWND m_editMainGrf = nullptr;
        HWND m_editGameExe = nullptr;
        HWND m_editGameArgs = nullptr;
        HWND m_editWindowWidth = nullptr;
        HWND m_editWindowHeight = nullptr;
        HWND m_listElements = nullptr;

        // Propriedades do elemento selecionado
        HWND m_editElemX = nullptr;
        HWND m_editElemY = nullptr;
        HWND m_editElemW = nullptr;
        HWND m_editElemH = nullptr;
        HWND m_editElemText = nullptr;
        HWND m_comboElemAction = nullptr;

        // Status
        std::wstring m_statusText = L"Pronto para configurar";
        int m_progress = 0;

        // Tracking
        TRACKMOUSEEVENT m_trackMouse = {};
        bool m_isTracking = false;

        // Flag para evitar loops de atualização
        bool m_updatingProperties = false;
    };

} // namespace autopatch
