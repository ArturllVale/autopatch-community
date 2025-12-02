// embedded_browser.h - Simple embedded IE WebBrowser control
// Uses MSHTML/Trident engine (no external dependencies)
//
// AutoPatch Community
// Copyright (C) 2024 - Cremané (saadrcaa@gmail.com)
// Licensed under MIT License

#pragma once

#include <Windows.h>
#include <string>

namespace autopatch
{

    // Forward declaration
    class SimpleOleContainer;

    // Classe simples para criar um controle WebBrowser embarcado
    // Usa o motor do Internet Explorer (MSHTML) que já vem com o Windows
    class EmbeddedBrowser
    {
    public:
        EmbeddedBrowser();
        ~EmbeddedBrowser();

        // Cria o controle WebBrowser como janela filha
        bool Create(HWND parentHwnd, int x, int y, int width, int height);

        // Navega para uma URL
        bool Navigate(const std::wstring &url);

        // Obtém handle da janela container
        HWND GetHwnd() const { return m_hwnd; }

        // Redimensiona o controle
        void Resize(int width, int height);

        // Destrói o controle
        void Destroy();

    private:
        HWND m_hwnd = nullptr;
        SimpleOleContainer *m_pContainer = nullptr;
    };

    // Função helper para criar um WebBrowser e navegar para URL
    HWND CreateEmbeddedBrowser(HWND parent, int x, int y, int w, int h, const wchar_t *url);

} // namespace autopatch
