#pragma once

#include <Windows.h>
#include <string>
#include <memory>
#include "ui.h"
#include "../core/config.h"
#include "../core/patcher.h"

namespace autopatch
{

    class MainWindow
    {
    public:
        MainWindow();
        ~MainWindow();

        // Inicializa e cria a janela
        bool Create(HINSTANCE hInstance);

        // Loop de mensagens
        int Run();

        // Obtém handle da janela
        HWND GetHandle() const { return m_hwnd; }

        // Obtém instância singleton
        static MainWindow *GetInstance() { return s_instance; }

    private:
        // Window procedure
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        // Handlers de mensagens
        void OnCreate();
        void OnDestroy();
        void OnPaint();
        void OnMouseMove(int x, int y);
        void OnLButtonDown(int x, int y);
        void OnLButtonUp(int x, int y);
        void OnTimer(UINT_PTR timerId);

        // Ações
        void OnButtonAction(const std::wstring &action);
        void StartGame();
        void CheckFiles();
        void OpenSettings();
        void MinimizeWindow();
        void CloseWindow();

        // Atualiza status
        void SetStatus(const std::wstring &text);
        void SetProgress(float progress);

        // Inicia verificação de patches
        void StartPatchCheck();

        HWND m_hwnd = nullptr;
        HINSTANCE m_hInstance = nullptr;

        std::unique_ptr<UI> m_ui;
        PatcherConfig m_config;
        std::unique_ptr<Patcher> m_patcher;

        bool m_dragging = false;
        POINT m_dragStart = {};

        static MainWindow *s_instance;
        static constexpr UINT_PTR TIMER_UPDATE = 1;
    };

} // namespace autopatch
