// AutoPatch Builder - Main Entry Point
// C++ Native Application
//
// AutoPatch Community
// Copyright (C) 2024 - Cremané (saadrcaa@gmail.com)
// Licensed under MIT License

#include <Windows.h>
#include <objbase.h>
#include <gdiplus.h>
#include <CommCtrl.h>
#include "builder_window.h"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // Inicializa GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // Inicializa Common Controls v6
    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES |
                 ICC_PROGRESS_CLASS | ICC_BAR_CLASSES | ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&icex);

    // Inicializa COM (necessário para alguns controles)
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // Cria e executa a janela do Builder
    autopatch::ModernBuilderWindow window;

    if (!window.Create(hInstance))
    {
        MessageBoxW(nullptr, L"Failed to create window!", L"Error", MB_ICONERROR);
        CoUninitialize();
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }

    int result = window.Run();

    CoUninitialize();
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return result;
}
