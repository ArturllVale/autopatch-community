// AutoPatcher - Main Entry Point
// C++ Native Application
//
// AutoPatch Community
// Copyright (C) 2024 - Cremané (saadrcaa@gmail.com)
// Licensed under MIT License

#include "window.h"
#include "skin.h"
#include "mshtml_window.h"
#include "../core/config.h"
#include "../core/resources.h"
#include "../core/utils.h"
#include "../core/patcher.h"
#include <Windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <thread>
#include <atomic>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Variáveis globais para controle do patcher no modo HTML
static std::atomic<bool> g_patchingComplete{false};
static std::atomic<bool> g_patchingError{false};
static std::wstring g_lastErrorMessage;

// Função para iniciar o jogo
void StartGame(const autopatch::PatcherConfig &config, HWND hwnd)
{
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    std::wstring exePath = autopatch::utils::StringToWide(config.clientExe);
    std::wstring cmdLine = exePath;
    if (!config.clientArgs.empty())
    {
        cmdLine += L" " + autopatch::utils::StringToWide(config.clientArgs);
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
        PostQuitMessage(0);
    }
    else
    {
        // Tenta ShellExecuteW como fallback
        HINSTANCE result = ShellExecuteW(hwnd, L"open", exePath.c_str(),
                                         config.clientArgs.empty() ? nullptr : autopatch::utils::StringToWide(config.clientArgs).c_str(),
                                         currentDir, SW_SHOW);

        if ((INT_PTR)result > 32)
        {
            // Sucesso com ShellExecute
            PostQuitMessage(0);
        }
        else
        {
            // Mostra erro detalhado
            DWORD error = GetLastError();
            wchar_t msg[512];
            swprintf_s(msg, L"Falha ao iniciar o jogo.\n\nExecutável: %s\nDiretório: %s\nErro: %lu",
                       exePath.c_str(), currentDir, error);
            MessageBoxW(hwnd, msg, L"Erro", MB_ICONERROR);
        }
    }
}

// Classe para gerenciar o patcher no modo HTML
template <typename WindowType>
class HtmlPatcherController
{
public:
    HtmlPatcherController(WindowType &window, const autopatch::PatcherConfig &config)
        : m_window(window), m_config(config)
    {
        m_patcher.Initialize(config);
    }

    void StartPatchCheck()
    {
        // Verifica se tem URL de patchlist configurada
        if (m_config.patchListUrl.empty())
        {
            // Sem URL configurada - habilita o botão direto
            m_window.SetProgress(100, L"Pronto para jogar!");
            m_window.EnableStartButton(true);
            g_patchingComplete = true;
            return;
        }

        m_patcher.SetProgressCallback([this](autopatch::PatcherStatus status,
                                             const std::wstring &message,
                                             float progress)
                                      {
            int percent = static_cast<int>(progress * 100);
            
            switch (status)
            {
            case autopatch::PatcherStatus::CheckingUpdates:
                m_window.SetProgress(percent, L"Verificando atualizações...");
                break;
                
            case autopatch::PatcherStatus::Downloading:
                m_window.SetProgress(percent, message);
                break;
                
            case autopatch::PatcherStatus::Patching:
                m_window.SetProgress(percent, message);
                break;
                
            case autopatch::PatcherStatus::Complete:
                m_window.SetProgress(100, L"Atualização concluída!");
                m_window.EnableStartButton(true);
                g_patchingComplete = true;
                break;
                
            case autopatch::PatcherStatus::Error:
                m_window.SetProgress(0, L"Erro: " + message);
                m_window.EnableStartButton(true); // Permite jogar mesmo com erro
                g_patchingError = true;
                g_lastErrorMessage = message;
                
                // Mostra detalhes do erro
                {
                    std::wstring errorMsg = L"Erro ao verificar atualizações:\n\n" + message;
                    errorMsg += L"\n\nURL: " + autopatch::utils::StringToWide(m_config.patchListUrl);
                    errorMsg += L"\n\nVocê pode continuar jogando, mas pode haver atualizações pendentes.";
                    MessageBoxW(m_window.GetHwnd(), errorMsg.c_str(), L"Aviso", MB_ICONWARNING);
                }
                break;
                
            default:
                break;
            } });

        // Inicia verificação em thread separada
        m_patcher.CheckForUpdates();
    }

private:
    WindowType &m_window;
    const autopatch::PatcherConfig &m_config;
    autopatch::Patcher m_patcher;
};

// Tenta renderizar HTML usando MSHTML (IE WebBrowser control)
bool TryRunHtmlMode(HINSTANCE hInstance, const autopatch::PatcherConfig &config)
{
    // Carrega o skin para obter HTML/CSS/JS
    autopatch::Skin &skin = autopatch::GetSkin();
    if (!skin.LoadFromResources())
    {
        MessageBoxW(nullptr, L"Falha ao carregar recursos do skin", L"Erro", MB_ICONERROR);
        return false;
    }

    // Verifica se há conteúdo HTML
    if (skin.GetHtmlContent().empty())
    {
        MessageBoxW(nullptr, L"Conteúdo HTML não encontrado nos recursos", L"Erro", MB_ICONERROR);
        return false;
    }

    // Usa MSHTML (Internet Explorer WebBrowser control)
    // Funciona em Windows 7/8/10/11 sem criar arquivos extras
    autopatch::MshtmlWindow mshtmlWindow;

    if (!mshtmlWindow.Create(hInstance, config.windowWidth, config.windowHeight,
                             autopatch::utils::StringToWide(config.serverName)))
    {
        MessageBoxW(nullptr, L"Falha ao criar janela MSHTML", L"Erro", MB_ICONERROR);
        return false;
    }

    // DEBUG: Mostra que a janela foi criada
    OutputDebugStringW(L"[DEBUG] MshtmlWindow criada com sucesso\n");

    if (!mshtmlWindow.LoadContent(skin.GetHtmlContent(), skin.GetCssContent(), skin.GetJsContent()))
    {
        MessageBoxW(nullptr, L"Falha ao carregar conteúdo HTML", L"Erro", MB_ICONERROR);
        return false;
    }

    // DEBUG: Mostra que o conteúdo foi carregado
    OutputDebugStringW(L"[DEBUG] Conteúdo HTML carregado\n");

    // Copia a config para evitar problemas de lifetime
    static autopatch::PatcherConfig s_config = config;
    static autopatch::MshtmlWindow *s_window = &mshtmlWindow;

    mshtmlWindow.SetStartGameCallback([]()
                                      { StartGame(s_config, s_window->GetHwnd()); });

    mshtmlWindow.SetCloseCallback([]()
                                  { PostQuitMessage(0); });

    // Cria o patcher estático para manter ele vivo durante o loop de mensagens
    static autopatch::Patcher s_patcher;
    s_patcher.Initialize(s_config);

    // Verifica se tem URL de patchlist configurada
    if (s_config.patchListUrl.empty())
    {
        // Sem URL configurada - habilita o botão direto
        mshtmlWindow.SetProgress(100, L"Pronto para jogar!");
        mshtmlWindow.EnableStartButton(true);
    }
    else
    {
        // Desabilita botão inicialmente
        mshtmlWindow.EnableStartButton(false);
        mshtmlWindow.SetProgress(0, L"Verificando atualizações...");

        s_patcher.SetProgressCallback([](autopatch::PatcherStatus status,
                                         const std::wstring &message,
                                         float progress)
                                      {
            int percent = static_cast<int>(progress * 100);
            
            switch (status)
            {
            case autopatch::PatcherStatus::CheckingUpdates:
                s_window->SetProgress(percent, L"Verificando atualizações...");
                break;
                
            case autopatch::PatcherStatus::Downloading:
                s_window->SetProgress(percent, message);
                break;
                
            case autopatch::PatcherStatus::Patching:
                s_window->SetProgress(percent, message);
                break;
                
            case autopatch::PatcherStatus::Complete:
                s_window->SetProgress(100, L"Atualização concluída!");
                s_window->EnableStartButton(true);
                break;
                
            case autopatch::PatcherStatus::Error:
                s_window->SetProgress(0, L"Erro: " + message);
                s_window->EnableStartButton(true); // Permite jogar mesmo com erro
                break;
                
            default:
                break;
            } });

        // Inicia verificação
        s_patcher.CheckForUpdates();
    }

    OutputDebugStringW(L"[DEBUG] Entrando no loop de mensagens...\n");
    mshtmlWindow.Run();
    OutputDebugStringW(L"[DEBUG] Saiu do loop de mensagens!\n");
    return true;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // Inicializa Common Controls
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    // Habilita DPI awareness
    SetProcessDPIAware();

    // Inicializa COM para MSHTML
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // Carrega configuração para verificar o modo
    auto config = autopatch::LoadConfig();

    // Se for modo HTML, tenta usar MSHTML
    if (config.uiType == autopatch::UIType::Html)
    {
        if (TryRunHtmlMode(hInstance, config))
        {
            CoUninitialize();
            return 0;
        }
        // Se falhar, continua para o fallback GDI+
    }

    // Modo Image ou fallback - usa a janela GDI+ padrão
    autopatch::MainWindow mainWindow;

    if (!mainWindow.Create(hInstance))
    {
        CoUninitialize();
        return 1;
    }

    int result = mainWindow.Run();
    CoUninitialize();
    return result;
}
