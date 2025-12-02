#pragma once

#include <Windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>

#pragma comment(lib, "gdiplus.lib")

namespace autopatch
{
    // Cores do tema escuro (igual ao C#)
    namespace Colors
    {
        const COLORREF Primary = RGB(30, 30, 30);          // #1E1E1E
        const COLORREF Secondary = RGB(37, 37, 38);        // #252526
        const COLORREF Tertiary = RGB(45, 45, 48);         // #2D2D30
        const COLORREF Border = RGB(63, 63, 70);           // #3F3F46
        const COLORREF Accent = RGB(0, 122, 204);          // #007ACC
        const COLORREF AccentHover = RGB(28, 151, 234);    // #1C97EA
        const COLORREF Text = RGB(204, 204, 204);          // #CCCCCC
        const COLORREF TextSecondary = RGB(128, 128, 128); // #808080
        const COLORREF Success = RGB(78, 201, 176);        // #4EC9B0
        const COLORREF Error = RGB(241, 76, 76);           // #F14C4C
        const COLORREF White = RGB(255, 255, 255);
    }

    // Estrutura para elemento UI
    struct UIElementData
    {
        std::string id;
        int type = 0; // 0=Button, 1=Label, 2=ProgressBar
        int x = 0, y = 0;
        int width = 100, height = 30;
        std::wstring text;
        std::string action;
        std::wstring normalImage;
        std::wstring hoverImage;
        std::wstring pressedImage;
        std::wstring fontFamily = L"Segoe UI";
        int fontSize = 12;
        COLORREF fontColor = Colors::White;
        COLORREF bgColor = Colors::Tertiary;
        COLORREF fillColor = Colors::Accent;
        bool isSelected = false;
    };

    // Projeto do patcher
    struct PatcherProjectData
    {
        std::wstring serverName = L"Meu Servidor";
        std::wstring baseUrl = L"https://seuservidor.com/patch/";
        std::wstring patchlistFile = L"patchlist.txt";
        std::wstring patchesFolder = L"patches/";
        std::wstring mainGrf = L"data.grf";
        std::wstring gameExecutable = L"ragexe.exe";
        std::wstring gameArguments;
        bool closeAfterStart = true;
        int interfaceMode = 0; // 0=Image, 1=HTML
        int windowWidth = 800;
        int windowHeight = 600;

        // Image mode
        std::wstring backgroundImagePath;
        std::vector<uint8_t> backgroundImageData;
        std::vector<UIElementData> elements;

        // HTML mode
        std::wstring htmlContent;
        std::wstring cssContent;
        std::wstring jsContent;
    };

    // Controle base customizado
    class CustomControl
    {
    public:
        virtual ~CustomControl() = default;
        virtual void Paint(HDC hdc, const RECT &rc) = 0;
        virtual void OnMouseMove(int x, int y) {}
        virtual void OnMouseDown(int x, int y) {}
        virtual void OnMouseUp(int x, int y) {}
        virtual bool HitTest(int x, int y) const { return false; }

        RECT bounds = {};
        bool isHovered = false;
        bool isPressed = false;
        bool isEnabled = true;
        bool isVisible = true;
    };

    // Botão customizado estilo VS Code
    class ModernButton : public CustomControl
    {
    public:
        std::wstring text;
        std::function<void()> onClick;
        bool isPrimary = false;

        void Paint(HDC hdc, const RECT &rc) override;
        bool HitTest(int x, int y) const override;
        void OnMouseDown(int x, int y) override;
        void OnMouseUp(int x, int y) override;
    };

    // TextBox customizado
    class ModernTextBox : public CustomControl
    {
    public:
        std::wstring text;
        std::wstring placeholder;
        HWND hwndEdit = nullptr;

        void Create(HWND parent, HINSTANCE hInst, int x, int y, int w, int h);
        void Paint(HDC hdc, const RECT &rc) override;
        std::wstring GetText() const;
        void SetText(const std::wstring &t);
    };

    // CheckBox customizado
    class ModernCheckBox : public CustomControl
    {
    public:
        std::wstring text;
        bool isChecked = false;
        std::function<void(bool)> onChanged;

        void Paint(HDC hdc, const RECT &rc) override;
        bool HitTest(int x, int y) const override;
        void OnMouseUp(int x, int y) override;
    };

    // Card de seleção de modo
    class ModeCard : public CustomControl
    {
    public:
        std::wstring title;
        std::wstring description;
        std::wstring icon;
        bool isSelected = false;
        std::function<void()> onClick;

        void Paint(HDC hdc, const RECT &rc) override;
        bool HitTest(int x, int y) const override;
        void OnMouseUp(int x, int y) override;
    };

    // Canvas de design para arrastar elementos
    class DesignCanvas : public CustomControl
    {
    public:
        PatcherProjectData *project = nullptr;
        UIElementData *selectedElement = nullptr;
        std::function<void(UIElementData *)> onSelectionChanged;

        // Para drag
        bool isDragging = false;
        int dragStartX = 0, dragStartY = 0;
        int elementStartX = 0, elementStartY = 0;

        // Escala e offset calculados no Paint
        float currentScale = 1.0f;
        int currentOffsetX = 0;
        int currentOffsetY = 0;

        void Paint(HDC hdc, const RECT &rc) override;
        void PaintElement(HDC hdc, UIElementData &elem, int offsetX, int offsetY, float scale);
        bool HitTest(int x, int y) const override;
        void OnMouseDown(int x, int y) override;
        void OnMouseMove(int x, int y) override;
        void OnMouseUp(int x, int y) override;
        UIElementData *HitTestElement(int x, int y);

        void LoadBackgroundImage(const std::wstring &path);
        Gdiplus::Image *backgroundImage = nullptr;
    };

    // Funções auxiliares de desenho
    namespace Drawing
    {
        void FillRect(HDC hdc, const RECT &rc, COLORREF color);
        void DrawRect(HDC hdc, const RECT &rc, COLORREF color, int thickness = 1);
        void DrawRoundRect(HDC hdc, const RECT &rc, COLORREF fillColor, COLORREF borderColor, int radius);
        void DrawText(HDC hdc, const std::wstring &text, const RECT &rc, COLORREF color,
                      int fontSize = 12, const wchar_t *fontName = L"Segoe UI",
                      UINT format = DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        void DrawTextBold(HDC hdc, const std::wstring &text, const RECT &rc, COLORREF color,
                          int fontSize = 12, const wchar_t *fontName = L"Segoe UI",
                          UINT format = DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        HBRUSH CreateSolidBrushCached(COLORREF color);
    }

} // namespace autopatch
