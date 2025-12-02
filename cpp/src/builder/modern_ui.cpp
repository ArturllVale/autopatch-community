#include "modern_ui.h"
#include <map>
#include <algorithm>

namespace autopatch
{
    // Cache de brushes
    static std::map<COLORREF, HBRUSH> s_brushCache;

    namespace Drawing
    {
        HBRUSH CreateSolidBrushCached(COLORREF color)
        {
            auto it = s_brushCache.find(color);
            if (it != s_brushCache.end())
                return it->second;

            HBRUSH brush = CreateSolidBrush(color);
            s_brushCache[color] = brush;
            return brush;
        }

        void FillRect(HDC hdc, const RECT &rc, COLORREF color)
        {
            HBRUSH brush = CreateSolidBrushCached(color);
            ::FillRect(hdc, &rc, brush);
        }

        void DrawRect(HDC hdc, const RECT &rc, COLORREF color, int thickness)
        {
            HPEN pen = CreatePen(PS_SOLID, thickness, color);
            HPEN oldPen = (HPEN)SelectObject(hdc, pen);
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

            Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);
        }

        void DrawRoundRect(HDC hdc, const RECT &rc, COLORREF fillColor, COLORREF borderColor, int radius)
        {
            HBRUSH brush = CreateSolidBrushCached(fillColor);
            HPEN pen = CreatePen(PS_SOLID, 1, borderColor);

            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
            HPEN oldPen = (HPEN)SelectObject(hdc, pen);

            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);

            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);
        }

        void DrawText(HDC hdc, const std::wstring &text, const RECT &rc, COLORREF color,
                      int fontSize, const wchar_t *fontName, UINT format)
        {
            HFONT font = CreateFontW(
                -MulDiv(fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72),
                0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontName);

            HFONT oldFont = (HFONT)SelectObject(hdc, font);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, color);

            RECT rcText = rc;
            ::DrawTextW(hdc, text.c_str(), -1, &rcText, format);

            SelectObject(hdc, oldFont);
            DeleteObject(font);
        }

        void DrawTextBold(HDC hdc, const std::wstring &text, const RECT &rc, COLORREF color,
                          int fontSize, const wchar_t *fontName, UINT format)
        {
            HFONT font = CreateFontW(
                -MulDiv(fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72),
                0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontName);

            HFONT oldFont = (HFONT)SelectObject(hdc, font);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, color);

            RECT rcText = rc;
            ::DrawTextW(hdc, text.c_str(), -1, &rcText, format);

            SelectObject(hdc, oldFont);
            DeleteObject(font);
        }
    }

    // ModernButton implementation
    void ModernButton::Paint(HDC hdc, const RECT &rc)
    {
        if (!isVisible)
            return;

        COLORREF bgColor, textColor, borderColor;

        if (!isEnabled)
        {
            bgColor = Colors::Tertiary;
            textColor = Colors::TextSecondary;
            borderColor = Colors::Border;
        }
        else if (isPrimary)
        {
            bgColor = isPressed ? RGB(0, 90, 158) : (isHovered ? Colors::AccentHover : Colors::Accent);
            textColor = Colors::White;
            borderColor = bgColor;
        }
        else
        {
            bgColor = isPressed ? Colors::Border : (isHovered ? Colors::Border : Colors::Tertiary);
            textColor = Colors::Text;
            borderColor = Colors::Border;
        }

        Drawing::DrawRoundRect(hdc, rc, bgColor, borderColor, 6);
        Drawing::DrawText(hdc, text, rc, textColor, 13, L"Segoe UI", DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    bool ModernButton::HitTest(int x, int y) const
    {
        return isVisible && isEnabled &&
               x >= bounds.left && x < bounds.right &&
               y >= bounds.top && y < bounds.bottom;
    }

    void ModernButton::OnMouseDown(int x, int y)
    {
        if (HitTest(x, y))
            isPressed = true;
    }

    void ModernButton::OnMouseUp(int x, int y)
    {
        if (isPressed && HitTest(x, y) && onClick)
            onClick();
        isPressed = false;
    }

    // ModernTextBox implementation
    void ModernTextBox::Create(HWND parent, HINSTANCE hInst, int x, int y, int w, int h)
    {
        bounds = {x, y, x + w, y + h};

        hwndEdit = CreateWindowExW(
            0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            x + 8, y + 4, w - 16, h - 8,
            parent, nullptr, hInst, nullptr);

        // Fonte moderna
        HFONT font = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        SendMessage(hwndEdit, WM_SETFONT, (WPARAM)font, TRUE);
    }

    void ModernTextBox::Paint(HDC hdc, const RECT &rc)
    {
        Drawing::DrawRoundRect(hdc, rc, Colors::Tertiary, Colors::Border, 6);
    }

    std::wstring ModernTextBox::GetText() const
    {
        if (!hwndEdit)
            return L"";
        int len = GetWindowTextLengthW(hwndEdit);
        std::wstring text(len + 1, L'\0');
        GetWindowTextW(hwndEdit, &text[0], len + 1);
        text.resize(len);
        return text;
    }

    void ModernTextBox::SetText(const std::wstring &t)
    {
        if (hwndEdit)
            SetWindowTextW(hwndEdit, t.c_str());
    }

    // ModernCheckBox implementation
    void ModernCheckBox::Paint(HDC hdc, const RECT &rc)
    {
        if (!isVisible)
            return;

        // Caixa de check
        RECT checkBox = {rc.left, rc.top + 2, rc.left + 18, rc.top + 20};
        COLORREF boxBg = isChecked ? Colors::Accent : Colors::Tertiary;
        COLORREF boxBorder = isChecked ? Colors::Accent : Colors::Border;

        Drawing::DrawRoundRect(hdc, checkBox, boxBg, boxBorder, 4);

        // Checkmark usando X simples
        if (isChecked)
        {
            HPEN pen = CreatePen(PS_SOLID, 2, Colors::White);
            HPEN oldPen = (HPEN)SelectObject(hdc, pen);

            // Desenha V
            MoveToEx(hdc, rc.left + 4, rc.top + 10, nullptr);
            LineTo(hdc, rc.left + 8, rc.top + 15);
            LineTo(hdc, rc.left + 14, rc.top + 6);

            SelectObject(hdc, oldPen);
            DeleteObject(pen);
        }

        // Texto
        RECT textRect = {rc.left + 26, rc.top, rc.right, rc.bottom};
        Drawing::DrawText(hdc, text, textRect, Colors::Text, 13);
    }

    bool ModernCheckBox::HitTest(int x, int y) const
    {
        return isVisible && x >= bounds.left && x < bounds.right &&
               y >= bounds.top && y < bounds.bottom;
    }

    void ModernCheckBox::OnMouseUp(int x, int y)
    {
        if (HitTest(x, y))
        {
            isChecked = !isChecked;
            if (onChanged)
                onChanged(isChecked);
        }
    }

    // ModeCard implementation
    void ModeCard::Paint(HDC hdc, const RECT &rc)
    {
        if (!isVisible)
            return;

        COLORREF bgColor = isSelected ? Colors::Accent : Colors::Tertiary;
        COLORREF borderColor = isSelected ? Colors::Accent : Colors::Border;
        int borderThickness = isSelected ? 2 : 1;

        if (isHovered && !isSelected)
            bgColor = Colors::Border;

        Drawing::DrawRoundRect(hdc, rc, bgColor, borderColor, 8);

        // Ícone
        RECT iconRect = {rc.left + 12, rc.top + 12, rc.left + 40, rc.top + 40};
        Drawing::DrawText(hdc, icon, iconRect, Colors::White, 18);

        // Título
        RECT titleRect = {rc.left + 42, rc.top + 12, rc.right - 12, rc.top + 32};
        Drawing::DrawTextBold(hdc, title, titleRect, Colors::White, 13);

        // Descrição
        RECT descRect = {rc.left + 42, rc.top + 32, rc.right - 12, rc.bottom - 8};
        Drawing::DrawText(hdc, description, descRect, Colors::TextSecondary, 11, L"Segoe UI",
                          DT_LEFT | DT_WORDBREAK);
    }

    bool ModeCard::HitTest(int x, int y) const
    {
        return isVisible && x >= bounds.left && x < bounds.right &&
               y >= bounds.top && y < bounds.bottom;
    }

    void ModeCard::OnMouseUp(int x, int y)
    {
        if (HitTest(x, y) && onClick)
            onClick();
    }

    // DesignCanvas implementation
    void DesignCanvas::Paint(HDC hdc, const RECT &rc)
    {
        // Fundo do canvas
        Drawing::FillRect(hdc, rc, RGB(26, 26, 26));

        if (!project)
            return;

        // Calcula área do patcher preview
        int canvasW = rc.right - rc.left;
        int canvasH = rc.bottom - rc.top;
        int patcherW = project->windowWidth;
        int patcherH = project->windowHeight;

        // Escala para caber no canvas
        float scale = (std::min)((float)(canvasW - 40) / patcherW, (float)(canvasH - 40) / patcherH);
        if (scale > 1.0f)
            scale = 1.0f;

        int scaledW = (int)(patcherW * scale);
        int scaledH = (int)(patcherH * scale);
        int offsetX = rc.left + (canvasW - scaledW) / 2;
        int offsetY = rc.top + (canvasH - scaledH) / 2;

        // Área do patcher
        RECT patcherRect = {offsetX, offsetY, offsetX + scaledW, offsetY + scaledH};
        Drawing::FillRect(hdc, patcherRect, Colors::Primary);
        Drawing::DrawRect(hdc, patcherRect, Colors::Border, 1);

        // Background image
        if (backgroundImage)
        {
            Gdiplus::Graphics graphics(hdc);
            graphics.DrawImage(backgroundImage, offsetX, offsetY, scaledW, scaledH);
        }

        // Guarda escala para uso em PaintElement
        currentScale = scale;
        currentOffsetX = offsetX;
        currentOffsetY = offsetY;

        // Desenha elementos
        for (auto &elem : project->elements)
        {
            PaintElement(hdc, elem, offsetX, offsetY, scale);
        }
    }

    void DesignCanvas::PaintElement(HDC hdc, UIElementData &elem, int offsetX, int offsetY, float scale)
    {
        int x = offsetX + (int)(elem.x * scale);
        int y = offsetY + (int)(elem.y * scale);
        int w = (int)(elem.width * scale);
        int h = (int)(elem.height * scale);
        RECT elemRect = {x, y, x + w, y + h};

        switch (elem.type)
        {
        case 0: // Button
            Drawing::DrawRoundRect(hdc, elemRect, Colors::Accent, Colors::Accent, 4);
            Drawing::DrawText(hdc, elem.text, elemRect, Colors::White, (int)(12 * scale), L"Segoe UI",
                              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            break;

        case 1: // Label generico
            Drawing::DrawText(hdc, elem.text, elemRect, elem.fontColor, (int)(elem.fontSize * scale),
                              elem.fontFamily.c_str(), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            break;

        case 2: // ProgressBar
            Drawing::FillRect(hdc, elemRect, elem.bgColor);
            {
                RECT fillRect = elemRect;
                fillRect.right = fillRect.left + (fillRect.right - fillRect.left) * 65 / 100;
                Drawing::FillRect(hdc, fillRect, elem.fillColor);
            }
            break;

        case 3: // Status Label
            Drawing::DrawRoundRect(hdc, elemRect, RGB(40, 40, 40), RGB(80, 80, 80), 2);
            Drawing::DrawText(hdc, L"[Status] " + elem.text, elemRect, RGB(0, 255, 128), (int)(11 * scale), L"Segoe UI",
                              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            break;

        case 4: // Percentagem Label
            Drawing::DrawRoundRect(hdc, elemRect, RGB(40, 40, 40), RGB(80, 80, 80), 2);
            Drawing::DrawText(hdc, L"[%] " + elem.text, elemRect, RGB(255, 200, 0), (int)(11 * scale), L"Segoe UI",
                              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            break;
        }

        // Seleção
        if (elem.isSelected)
        {
            RECT selRect = {elemRect.left - 2, elemRect.top - 2,
                            elemRect.right + 2, elemRect.bottom + 2};
            Drawing::DrawRect(hdc, selRect, RGB(255, 255, 0), 2);
        }
    }

    bool DesignCanvas::HitTest(int x, int y) const
    {
        return x >= bounds.left && x < bounds.right &&
               y >= bounds.top && y < bounds.bottom;
    }

    void DesignCanvas::OnMouseDown(int x, int y)
    {
        if (!project || !HitTest(x, y))
            return;

        // Verifica se clicou em algum elemento
        auto *elem = HitTestElement(x, y);

        // Deseleciona todos
        for (auto &e : project->elements)
            e.isSelected = false;

        if (elem)
        {
            elem->isSelected = true;
            selectedElement = elem;
            isDragging = true;
            dragStartX = x;
            dragStartY = y;
            elementStartX = elem->x;
            elementStartY = elem->y;
        }
        else
        {
            selectedElement = nullptr;
        }

        if (onSelectionChanged)
            onSelectionChanged(selectedElement);
    }

    void DesignCanvas::OnMouseMove(int x, int y)
    {
        if (isDragging && selectedElement && currentScale > 0)
        {
            // Converte delta de pixels para coordenadas do patcher
            int dx = (int)((x - dragStartX) / currentScale);
            int dy = (int)((y - dragStartY) / currentScale);
            selectedElement->x = elementStartX + dx;
            selectedElement->y = elementStartY + dy;

            // Clamp - garante que elemento fica dentro da area do patcher
            if (selectedElement->x < 0)
                selectedElement->x = 0;
            if (selectedElement->y < 0)
                selectedElement->y = 0;
            if (project)
            {
                int maxX = project->windowWidth - selectedElement->width;
                int maxY = project->windowHeight - selectedElement->height;
                if (maxX < 0)
                    maxX = 0;
                if (maxY < 0)
                    maxY = 0;
                if (selectedElement->x > maxX)
                    selectedElement->x = maxX;
                if (selectedElement->y > maxY)
                    selectedElement->y = maxY;
            }
        }
    }

    void DesignCanvas::OnMouseUp(int x, int y)
    {
        isDragging = false;
    }

    UIElementData *DesignCanvas::HitTestElement(int x, int y)
    {
        if (!project || currentScale <= 0)
            return nullptr;

        // Usa offset e escala calculados no Paint
        int offsetX = currentOffsetX;
        int offsetY = currentOffsetY;
        float scale = currentScale;

        // Hit test reverso (elementos no topo primeiro)
        for (auto it = project->elements.rbegin(); it != project->elements.rend(); ++it)
        {
            int ex = offsetX + (int)(it->x * scale);
            int ey = offsetY + (int)(it->y * scale);
            int ew = (int)(it->width * scale);
            int eh = (int)(it->height * scale);
            if (x >= ex && x < ex + ew && y >= ey && y < ey + eh)
                return &(*it);
        }
        return nullptr;
    }

    void DesignCanvas::LoadBackgroundImage(const std::wstring &path)
    {
        if (backgroundImage)
        {
            delete backgroundImage;
            backgroundImage = nullptr;
        }

        if (!path.empty())
        {
            backgroundImage = Gdiplus::Image::FromFile(path.c_str());

            // Atualiza dimensões do projeto para corresponder à imagem
            if (backgroundImage && backgroundImage->GetLastStatus() == Gdiplus::Ok && project)
            {
                project->windowWidth = backgroundImage->GetWidth();
                project->windowHeight = backgroundImage->GetHeight();
            }
        }
    }

} // namespace autopatch
