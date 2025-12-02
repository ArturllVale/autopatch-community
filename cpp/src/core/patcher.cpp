#include "patcher.h"
#include "http.h"
#include "grf.h"
#include "thor.h"
#include "utils.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <Windows.h>
#include <shellapi.h>

namespace autopatch
{

    Patcher::Patcher() = default;

    Patcher::~Patcher()
    {
        Cancel();
        if (m_workerThread.joinable())
        {
            m_workerThread.join();
        }
    }

    bool Patcher::Initialize(const PatcherConfig &config)
    {
        m_patchListUrl = config.patchListUrl;
        m_grfFiles = config.grfFiles;
        m_clientExe = config.clientExe;
        m_clientArgs = config.clientArgs;
        return !m_patchListUrl.empty();
    }

    void Patcher::CheckForUpdates()
    {
        if (IsBusy())
            return;

        m_cancelRequested = false;
        m_status = PatcherStatus::CheckingUpdates;

        m_workerThread = std::thread(&Patcher::WorkerThread, this);
    }

    void Patcher::ApplyPatches()
    {
        if (IsBusy())
            return;

        m_cancelRequested = false;
        m_status = PatcherStatus::Patching;

        // Aplica patches em thread separada
        m_workerThread = std::thread([this]()
                                     {
        for (size_t i = 0; i < m_pendingPatches.size() && !m_cancelRequested; i++) {
            const auto& patch = m_pendingPatches[i];
            
            float progress = static_cast<float>(i) / m_pendingPatches.size();
            std::wstring msg = L"Applying patch " + std::to_wstring(i + 1) + L" of " + 
                               std::to_wstring(m_pendingPatches.size());
            ReportProgress(PatcherStatus::Patching, msg, progress);
            
            ApplyPatch(patch);
        }
        
        if (!m_cancelRequested) {
            m_status = PatcherStatus::Complete;
            ReportProgress(PatcherStatus::Complete, L"Patching complete!", 1.0f);
        } });
    }

    void Patcher::Cancel()
    {
        m_cancelRequested = true;
    }

    void Patcher::SetProgressCallback(PatchProgressCallback callback)
    {
        m_progressCallback = std::move(callback);
    }

    bool Patcher::IsBusy() const
    {
        return m_status == PatcherStatus::CheckingUpdates ||
               m_status == PatcherStatus::Downloading ||
               m_status == PatcherStatus::Patching;
    }

    bool Patcher::StartGame()
    {
        if (m_clientExe.empty())
        {
            return false;
        }

        std::wstring exePath = utils::Utf8ToWide(m_clientExe);
        std::wstring args = utils::Utf8ToWide(m_clientArgs);

        SHELLEXECUTEINFOW sei = {};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb = L"open";
        sei.lpFile = exePath.c_str();
        sei.lpParameters = args.empty() ? nullptr : args.c_str();
        sei.nShow = SW_SHOW;

        return ShellExecuteExW(&sei) != FALSE;
    }

    void Patcher::WorkerThread()
    {
        ReportProgress(PatcherStatus::CheckingUpdates, L"Checking for updates...", 0.0f);

        // Carrega lista de patches já aplicados
        LoadAppliedPatches();

        // Baixa lista de patches
        DownloadPatchList();

        if (m_cancelRequested)
        {
            m_status = PatcherStatus::Idle;
            return;
        }

        if (m_pendingPatches.empty())
        {
            m_status = PatcherStatus::Complete;
            ReportProgress(PatcherStatus::Complete, L"No updates available", 1.0f);
            return;
        }

        // Baixa e aplica cada patch
        m_status = PatcherStatus::Downloading;

        for (size_t i = 0; i < m_pendingPatches.size() && !m_cancelRequested; i++)
        {
            const auto &patch = m_pendingPatches[i];

            float progress = static_cast<float>(i) / m_pendingPatches.size();
            std::wstring msg = L"Downloading " + utils::Utf8ToWide(patch.filename);
            ReportProgress(PatcherStatus::Downloading, msg, progress);

            DownloadPatch(patch);
        }

        if (m_cancelRequested)
        {
            m_status = PatcherStatus::Idle;
            return;
        }

        // Aplica patches
        m_status = PatcherStatus::Patching;

        for (size_t i = 0; i < m_pendingPatches.size() && !m_cancelRequested; i++)
        {
            const auto &patch = m_pendingPatches[i];

            float progress = static_cast<float>(i) / m_pendingPatches.size();
            std::wstring msg = L"Applying " + utils::Utf8ToWide(patch.filename);
            ReportProgress(PatcherStatus::Patching, msg, progress);

            ApplyPatch(patch);

            // Marca patch como aplicado e salva
            MarkPatchApplied(patch.filename);
            SaveAppliedPatches();
        }

        if (!m_cancelRequested)
        {
            m_status = PatcherStatus::Complete;
            ReportProgress(PatcherStatus::Complete, L"Update complete!", 1.0f);
        }
        else
        {
            m_status = PatcherStatus::Idle;
        }
    }

    void Patcher::DownloadPatchList()
    {
        HttpClient http;
        http.SetTimeout(30); // 30 segundos de timeout
        std::wstring url = utils::Utf8ToWide(m_patchListUrl);

        ReportProgress(PatcherStatus::CheckingUpdates, L"Conectando ao servidor...", 0.1f);

        auto response = http.Get(url);
        if (!response.success)
        {
            m_status = PatcherStatus::Error;
            std::wstring errorMsg = L"Falha ao baixar lista de patches";
            if (!response.error.empty())
            {
                errorMsg += L": " + response.error;
            }
            if (response.statusCode > 0)
            {
                errorMsg += L" (HTTP " + std::to_wstring(response.statusCode) + L")";
            }
            ReportProgress(PatcherStatus::Error, errorMsg, 0.0f);
            return;
        }

        if (response.body.empty())
        {
            m_status = PatcherStatus::Error;
            ReportProgress(PatcherStatus::Error, L"Lista de patches vazia ou inválida", 0.0f);
            return;
        }

        ReportProgress(PatcherStatus::CheckingUpdates, L"Analisando lista de patches...", 0.3f);

        // Parseia lista de patches
        m_pendingPatches.clear();

        std::string content = response.body;
        std::istringstream iss(content);
        std::string line;
        int index = 0;

        // Detecta base URL para patches
        std::string baseUrl = m_patchListUrl;
        size_t lastSlash = baseUrl.find_last_of('/');
        if (lastSlash != std::string::npos)
        {
            baseUrl = baseUrl.substr(0, lastSlash + 1);
        }

        while (std::getline(iss, line))
        {
            // Remove \r se houver
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            // Ignora linhas vazias e comentários
            if (line.empty() || line[0] == '#' || line[0] == '/')
                continue;

            PatchInfo patch;
            patch.index = index++;

            // Tenta formato 1: ID FILENAME [key=value ...]
            // Ex: 1 patch001.thor target=data.grf
            std::istringstream lineStream(line);
            std::string token;
            std::vector<std::string> tokens;

            while (lineStream >> token)
            {
                tokens.push_back(token);
            }

            if (tokens.size() >= 2)
            {
                // Verifica se primeiro token é número (ID)
                bool firstIsNumber = !tokens[0].empty() &&
                                     std::all_of(tokens[0].begin(), tokens[0].end(), ::isdigit);

                if (firstIsNumber)
                {
                    // Formato: ID FILENAME [key=value ...]
                    patch.filename = tokens[1];

                    // Parseia opções key=value
                    for (size_t i = 2; i < tokens.size(); i++)
                    {
                        size_t eqPos = tokens[i].find('=');
                        if (eqPos != std::string::npos)
                        {
                            std::string key = tokens[i].substr(0, eqPos);
                            std::string value = tokens[i].substr(eqPos + 1);

                            if (key == "target")
                            {
                                // Verifica se é GRF ou pasta
                                if (value.find(".grf") != std::string::npos)
                                {
                                    patch.targetGrf = value;
                                    patch.target = PatchTarget::GRF;
                                }
                                else
                                {
                                    patch.targetFolder = value;
                                    patch.target = PatchTarget::Folder;
                                }
                            }
                            else if (key == "hash" || key == "checksum")
                            {
                                patch.checksum = value;
                            }
                            else if (key == "size")
                            {
                                patch.size = std::stoull(value);
                            }
                            else if (key == "extract")
                            {
                                patch.extract = (value == "true" || value == "1");
                            }
                            else if (key == "folder")
                            {
                                patch.targetFolder = value;
                                patch.target = PatchTarget::Folder;
                            }
                        }
                    }
                }
                else
                {
                    // Tenta formato 2: filename|size|checksum
                    size_t pos1 = line.find('|');
                    size_t pos2 = line.find('|', pos1 + 1);

                    if (pos1 != std::string::npos)
                    {
                        patch.filename = line.substr(0, pos1);
                        if (pos2 != std::string::npos)
                        {
                            patch.size = std::stoull(line.substr(pos1 + 1, pos2 - pos1 - 1));
                            patch.checksum = line.substr(pos2 + 1);
                        }
                        else
                        {
                            patch.size = std::stoull(line.substr(pos1 + 1));
                        }
                    }
                    else
                    {
                        // Formato simples: apenas filename
                        patch.filename = tokens[0];
                    }
                }
            }
            else if (tokens.size() == 1)
            {
                // Apenas filename
                patch.filename = tokens[0];
            }
            else
            {
                continue; // Linha inválida
            }

            // Se não tiver target definido e tiver grfFiles, usa o primeiro
            if (patch.target == PatchTarget::Folder && patch.targetGrf.empty() && !m_grfFiles.empty())
            {
                // Verifica extensão do arquivo
                std::string ext = patch.filename;
                size_t dotPos = ext.find_last_of('.');
                if (dotPos != std::string::npos)
                {
                    ext = ext.substr(dotPos);
                    for (auto &c : ext)
                        c = tolower(c);

                    // Arquivos .thor e .rgz geralmente vão pro GRF
                    if (ext == ".thor" || ext == ".rgz" || ext == ".gpf")
                    {
                        patch.targetGrf = m_grfFiles[0];
                        patch.target = PatchTarget::GRF;
                    }
                }
            }

            // Constrói URL do patch se não for absoluta
            if (patch.filename.find("://") == std::string::npos)
            {
                patch.url = baseUrl + patch.filename;
            }
            else
            {
                patch.url = patch.filename;
            }

            // Verifica se patch já foi aplicado
            if (IsPatchApplied(patch.filename))
            {
                OutputDebugStringA(("[VERSION] Patch já aplicado, ignorando: " + patch.filename + "\n").c_str());
                continue;
            }

            m_pendingPatches.push_back(patch);
        }

        OutputDebugStringW((L"[VERSION] Patches pendentes (após filtro): " +
                            std::to_wstring(m_pendingPatches.size()) + L"\n")
                               .c_str());

        ReportProgress(PatcherStatus::CheckingUpdates,
                       L"Encontrados " + std::to_wstring(m_pendingPatches.size()) + L" patches pendentes", 0.5f);
    }

    void Patcher::DownloadPatch(const PatchInfo &patch)
    {
        HttpClient http;
        http.SetTimeout(120); // 2 minutos para downloads grandes
        std::wstring url = utils::Utf8ToWide(patch.url);
        std::wstring tempPath = utils::GetTempDirectory() + utils::Utf8ToWide(patch.filename);

        // Log para debug
        OutputDebugStringW((L"[PATCH] Baixando de: " + url + L"\n").c_str());
        OutputDebugStringW((L"[PATCH] Salvando em: " + tempPath + L"\n").c_str());

        bool success = http.DownloadFile(url, tempPath, [this, &patch](uint64_t downloaded, uint64_t total)
                                         {
        float progress = total > 0 ? static_cast<float>(downloaded) / total : 0.0f;
        std::wstring msg = L"Baixando " + utils::Utf8ToWide(patch.filename) + 
                           L" (" + utils::FormatFileSize(downloaded) + L" / " + 
                           utils::FormatFileSize(total) + L")";
        ReportProgress(PatcherStatus::Downloading, msg, progress); });

        if (!success)
        {
            m_status = PatcherStatus::Error;
            std::wstring msg = L"Falha ao baixar " + utils::Utf8ToWide(patch.filename);
            OutputDebugStringW((L"[PATCH] ERRO: " + msg + L"\n").c_str());
            ReportProgress(PatcherStatus::Error, msg, 0.0f);
        }
        else
        {
            OutputDebugStringW(L"[PATCH] Download concluído com sucesso\n");
        }
    }

    void Patcher::ApplyPatch(const PatchInfo &patch)
    {
        std::wstring tempPath = utils::GetTempDirectory() + utils::Utf8ToWide(patch.filename);

        OutputDebugStringW((L"[PATCH] Aplicando patch: " + utils::Utf8ToWide(patch.filename) + L"\n").c_str());
        OutputDebugStringW((L"[PATCH] Arquivo temp: " + tempPath + L"\n").c_str());

        // Verifica se arquivo existe
        if (GetFileAttributesW(tempPath.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            DWORD error = GetLastError();
            m_status = PatcherStatus::Error;
            std::wstring msg = L"Arquivo não encontrado: " + utils::Utf8ToWide(patch.filename) +
                               L" (erro " + std::to_wstring(error) + L")";
            OutputDebugStringW((L"[PATCH] ERRO: " + msg + L"\n").c_str());
            ReportProgress(PatcherStatus::Error, msg, 0.0f);
            return;
        }

        // Obtém extensão do arquivo
        std::wstring ext = utils::GetFileExtension(tempPath);
        for (auto &c : ext)
            c = towlower(c);

        OutputDebugStringW((L"[PATCH] Extensão: " + ext + L"\n").c_str());

        bool success = false;

        if (ext == L".thor")
        {
            // Aplica patch THOR ao GRF
            OutputDebugStringW(L"[PATCH] Aplicando como THOR\n");
            success = ApplyThorPatch(tempPath, patch);
        }
        else if (ext == L".rgz")
        {
            // Aplica patch RGZ (formato comprimido de múltiplos arquivos)
            OutputDebugStringW(L"[PATCH] Aplicando como RGZ\n");
            success = ApplyRgzPatch(tempPath, patch);
        }
        else if (ext == L".gpf")
        {
            // Aplica patch GPF (formato GRF patch)
            OutputDebugStringW(L"[PATCH] Aplicando como GPF\n");
            success = ApplyGpfPatch(tempPath, patch);
        }
        else if (ext == L".grf")
        {
            // Faz merge da GRF baixada com a GRF alvo
            OutputDebugStringW(L"[PATCH] Fazendo merge de GRF\n");
            success = MergeGrfPatch(tempPath, patch);
        }
        else
        {
            // Arquivo comum - copia para pasta de destino
            OutputDebugStringW(L"[PATCH] Copiando arquivo para pasta\n");
            success = CopyPatchToFolder(tempPath, patch);
        }

        // Remove arquivo temporário
        utils::DeleteFileW(tempPath);
    }

    bool Patcher::ApplyThorPatch(const std::wstring &tempPath, const PatchInfo &patch)
    {
        ThorFile thor;
        if (!thor.Open(tempPath))
        {
            m_status = PatcherStatus::Error;
            std::wstring msg = L"Falha ao abrir arquivo THOR: " + utils::Utf8ToWide(patch.filename);
            ReportProgress(PatcherStatus::Error, msg, 0.0f);
            return false;
        }

        bool success = false;

        // Verifica se deve fazer merge com GRF ou extrair para disco
        if (thor.UseGrfMerging())
        {
            OutputDebugStringW(L"[PATCH] THOR configurado para GRF merge\n");

            // Determina qual GRF usar
            std::string targetGrf;

            // Primeiro, verifica se o THOR especifica um GRF alvo
            if (!thor.GetTargetGrf().empty())
            {
                targetGrf = thor.GetTargetGrf();
                OutputDebugStringA(("[PATCH] THOR especifica GRF alvo: " + targetGrf + "\n").c_str());
            }
            // Senão, usa o do patch info ou o primeiro da configuração
            else if (!patch.targetGrf.empty())
            {
                targetGrf = patch.targetGrf;
                OutputDebugStringA(("[PATCH] Usando GRF do patch: " + targetGrf + "\n").c_str());
            }
            else if (!m_grfFiles.empty())
            {
                targetGrf = m_grfFiles[0];
                OutputDebugStringA(("[PATCH] Usando primeiro GRF da config: " + targetGrf + "\n").c_str());
            }

            if (targetGrf.empty())
            {
                OutputDebugStringW(L"[PATCH] ERRO: Nenhum GRF alvo definido\n");
                m_status = PatcherStatus::Error;
                ReportProgress(PatcherStatus::Error, L"Nenhum GRF alvo definido para o patch", 0.0f);
                return false;
            }

            // Constrói caminho completo do GRF (relativo ao diretório do app)
            std::wstring grfPath = utils::Utf8ToWide(targetGrf);
            if (grfPath.size() < 2 || grfPath[1] != L':')
            {
                // Caminho relativo - usa diretório do app
                grfPath = utils::GetAppDirectory() + L"\\" + grfPath;
            }

            OutputDebugStringW((L"[PATCH] Abrindo GRF: " + grfPath + L"\n").c_str());

            GrfFile grf;
            if (grf.Open(grfPath))
            {
                success = thor.ApplyTo(grf);
                if (success)
                {
                    OutputDebugStringW(L"[PATCH] THOR aplicado ao GRF com sucesso\n");
                }
                else
                {
                    OutputDebugStringW(L"[PATCH] ERRO: Falha ao aplicar THOR ao GRF\n");
                }
            }
            else
            {
                OutputDebugStringW((L"[PATCH] ERRO: Não foi possível abrir GRF: " + grfPath + L"\n").c_str());
            }
        }
        else
        {
            OutputDebugStringW(L"[PATCH] THOR configurado para extração no disco\n");

            // Extrai para o diretório do app (pasta do cliente)
            std::wstring outputDir = utils::GetAppDirectory();
            OutputDebugStringW((L"[PATCH] Extraindo para: " + outputDir + L"\n").c_str());

            success = thor.ApplyToDisk(outputDir);
            if (success)
            {
                OutputDebugStringW(L"[PATCH] THOR extraído para disco com sucesso\n");
            }
            else
            {
                OutputDebugStringW(L"[PATCH] ERRO: Falha ao extrair THOR para disco\n");
            }
        }

        if (!success)
        {
            m_status = PatcherStatus::Error;
            std::wstring msg = L"Falha ao aplicar patch THOR: " + utils::Utf8ToWide(patch.filename);
            ReportProgress(PatcherStatus::Error, msg, 0.0f);
        }

        return success;
    }

    bool Patcher::ApplyRgzPatch(const std::wstring &tempPath, const PatchInfo &patch)
    {
        // RGZ é um formato de arquivo comprimido que pode conter múltiplos arquivos
        // Por enquanto, trata como arquivo comum e copia para pasta
        // TODO: Implementar extração RGZ real
        return CopyPatchToFolder(tempPath, patch);
    }

    bool Patcher::ApplyGpfPatch(const std::wstring &tempPath, const PatchInfo &patch)
    {
        // GPF é um formato de patch GRF
        // Por enquanto, trata como arquivo comum
        // TODO: Implementar aplicação GPF real
        return CopyPatchToFolder(tempPath, patch);
    }

    bool Patcher::MergeGrfPatch(const std::wstring &tempPath, const PatchInfo &patch)
    {
        OutputDebugStringW((L"[PATCH] Iniciando merge de GRF: " + tempPath + L"\n").c_str());

        // Abre a GRF baixada (source)
        GrfFile sourceGrf;
        if (!sourceGrf.Open(tempPath))
        {
            OutputDebugStringW(L"[PATCH] ERRO: Não foi possível abrir GRF source\n");
            m_status = PatcherStatus::Error;
            ReportProgress(PatcherStatus::Error, L"Falha ao abrir GRF baixada: " + utils::Utf8ToWide(patch.filename), 0.0f);
            return false;
        }

        // Determina qual GRF de destino usar
        std::string targetGrfPath;
        if (!patch.targetGrf.empty())
        {
            targetGrfPath = patch.targetGrf;
        }
        else if (!m_grfFiles.empty())
        {
            targetGrfPath = m_grfFiles[0];
        }
        else
        {
            OutputDebugStringW(L"[PATCH] ERRO: Nenhuma GRF de destino configurada\n");
            m_status = PatcherStatus::Error;
            ReportProgress(PatcherStatus::Error, L"Nenhuma GRF de destino configurada", 0.0f);
            return false;
        }

        // Constrói caminho completo do GRF de destino
        std::wstring destGrfPath = utils::Utf8ToWide(targetGrfPath);
        if (destGrfPath.size() < 2 || destGrfPath[1] != L':')
        {
            // Caminho relativo - usa diretório do app
            destGrfPath = utils::GetAppDirectory() + L"\\" + destGrfPath;
        }

        OutputDebugStringW((L"[PATCH] GRF destino: " + destGrfPath + L"\n").c_str());

        // Abre a GRF de destino
        GrfFile destGrf;
        if (!destGrf.Open(destGrfPath))
        {
            OutputDebugStringW(L"[PATCH] ERRO: Não foi possível abrir GRF de destino\n");
            m_status = PatcherStatus::Error;
            ReportProgress(PatcherStatus::Error, L"Falha ao abrir GRF de destino: " + destGrfPath, 0.0f);
            return false;
        }

        // Obtém lista de arquivos da GRF source
        auto fileList = sourceGrf.GetFileList();
        OutputDebugStringW((L"[PATCH] Arquivos na GRF source: " + std::to_wstring(fileList.size()) + L"\n").c_str());

        int successCount = 0;
        int errorCount = 0;

        for (const auto &filename : fileList)
        {
            // Extrai arquivo da GRF source
            auto data = sourceGrf.ExtractFile(filename);
            if (data.empty())
            {
                OutputDebugStringA(("[PATCH] AVISO: Não foi possível extrair: " + filename + "\n").c_str());
                errorCount++;
                continue;
            }

            // Adiciona ao GRF de destino
            if (destGrf.AddFile(filename, data))
            {
                OutputDebugStringA(("[PATCH] Merged: " + filename + "\n").c_str());
                successCount++;
            }
            else
            {
                OutputDebugStringA(("[PATCH] ERRO ao adicionar: " + filename + "\n").c_str());
                errorCount++;
            }

            // Reporta progresso
            float progress = static_cast<float>(successCount + errorCount) / fileList.size();
            ReportProgress(PatcherStatus::Patching,
                           L"Merging GRF: " + std::to_wstring(successCount) + L"/" + std::to_wstring(fileList.size()),
                           progress);
        }

        OutputDebugStringW((L"[PATCH] Merge concluído: " + std::to_wstring(successCount) +
                            L" sucesso, " + std::to_wstring(errorCount) + L" erros\n")
                               .c_str());

        // Fecha a GRF source (não precisamos mais)
        sourceGrf.Close();

        // Salva as alterações na GRF de destino
        if (destGrf.Save())
        {
            // Fecha explicitamente para garantir que tudo foi escrito
            destGrf.Close();
            OutputDebugStringW(L"[PATCH] GRF de destino salva com sucesso\n");
            ReportProgress(PatcherStatus::Patching, L"GRF merged: " + utils::Utf8ToWide(patch.filename), 1.0f);
            return true;
        }
        else
        {
            destGrf.Close();
            OutputDebugStringW(L"[PATCH] ERRO: Falha ao salvar GRF de destino\n");
            m_status = PatcherStatus::Error;
            ReportProgress(PatcherStatus::Error, L"Falha ao salvar GRF de destino", 0.0f);
            return false;
        }
    }

    bool Patcher::CopyPatchToFolder(const std::wstring &tempPath, const PatchInfo &patch)
    {
        // Determina pasta de destino
        std::wstring destFolder;
        if (!patch.targetFolder.empty())
        {
            destFolder = utils::Utf8ToWide(patch.targetFolder);

            // Se for caminho relativo, usa diretório do patcher como base
            if (destFolder.size() < 2 || destFolder[1] != L':')
            {
                destFolder = utils::GetAppDirectory() + L"\\" + destFolder;
            }
        }
        else
        {
            // Usa diretório do patcher (onde está o .exe)
            destFolder = utils::GetAppDirectory();
        }

        // Garante que termina com barra
        if (!destFolder.empty() && destFolder.back() != L'\\' && destFolder.back() != L'/')
        {
            destFolder += L'\\';
        }

        // Extrai apenas o nome do arquivo (sem caminho)
        std::wstring filename = utils::Utf8ToWide(patch.filename);
        size_t lastSlash = filename.find_last_of(L"/\\");
        if (lastSlash != std::wstring::npos)
        {
            filename = filename.substr(lastSlash + 1);
        }

        std::wstring destPath = destFolder + filename;

        // Log para debug
        OutputDebugStringW((L"[PATCH] Copiando de: " + tempPath + L"\n").c_str());
        OutputDebugStringW((L"[PATCH] Para: " + destPath + L"\n").c_str());

        // Cria diretórios se necessário
        size_t pos = destPath.find_last_of(L"/\\");
        if (pos != std::wstring::npos)
        {
            std::wstring dir = destPath.substr(0, pos);
            CreateDirectoryRecursive(dir);
        }

        // Copia arquivo
        if (!CopyFileW(tempPath.c_str(), destPath.c_str(), FALSE))
        {
            DWORD error = GetLastError();
            m_status = PatcherStatus::Error;
            std::wstring msg = L"Falha ao copiar arquivo: " + filename + L" (erro " + std::to_wstring(error) + L")";
            ReportProgress(PatcherStatus::Error, msg, 0.0f);
            return false;
        }

        ReportProgress(PatcherStatus::Patching, L"Arquivo copiado: " + filename, 0.5f);
        return true;
    }

    void Patcher::CreateDirectoryRecursive(const std::wstring &path)
    {
        size_t pos = 0;
        while ((pos = path.find_first_of(L"/\\", pos + 1)) != std::wstring::npos)
        {
            std::wstring subPath = path.substr(0, pos);
            CreateDirectoryW(subPath.c_str(), nullptr);
        }
        CreateDirectoryW(path.c_str(), nullptr);
    }

    void Patcher::ReportProgress(PatcherStatus status, const std::wstring &message, float progress)
    {
        if (m_progressCallback)
        {
            m_progressCallback(status, message, progress);
        }
    }

    // =====================================================================
    // VERSION TRACKING
    // =====================================================================

    std::wstring Patcher::GetVersionFilePath() const
    {
        // Arquivo de versões fica na mesma pasta do patcher
        return utils::GetAppDirectory() + L"\\patcher.version";
    }

    void Patcher::LoadAppliedPatches()
    {
        m_appliedPatches.clear();

        std::wstring versionFile = GetVersionFilePath();
        OutputDebugStringW((L"[VERSION] Carregando versões de: " + versionFile + L"\n").c_str());

        std::ifstream file(versionFile);
        if (!file.is_open())
        {
            OutputDebugStringW(L"[VERSION] Arquivo de versões não existe (primeira execução)\n");
            return;
        }

        std::string line;
        while (std::getline(file, line))
        {
            // Remove \r se houver
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            if (!line.empty())
            {
                m_appliedPatches.insert(line);
                OutputDebugStringA(("[VERSION] Patch já aplicado: " + line + "\n").c_str());
            }
        }

        OutputDebugStringW((L"[VERSION] Total de patches já aplicados: " +
                            std::to_wstring(m_appliedPatches.size()) + L"\n")
                               .c_str());
    }

    void Patcher::SaveAppliedPatches()
    {
        std::wstring versionFile = GetVersionFilePath();
        OutputDebugStringW((L"[VERSION] Salvando versões em: " + versionFile + L"\n").c_str());

        std::ofstream file(versionFile, std::ios::trunc);
        if (!file.is_open())
        {
            OutputDebugStringW(L"[VERSION] ERRO: Não foi possível salvar arquivo de versões\n");
            return;
        }

        for (const auto &patch : m_appliedPatches)
        {
            file << patch << "\n";
        }

        OutputDebugStringW((L"[VERSION] Salvo " + std::to_wstring(m_appliedPatches.size()) +
                            L" patches aplicados\n")
                               .c_str());
    }

    bool Patcher::IsPatchApplied(const std::string &filename) const
    {
        return m_appliedPatches.find(filename) != m_appliedPatches.end();
    }

    void Patcher::MarkPatchApplied(const std::string &filename)
    {
        m_appliedPatches.insert(filename);
        OutputDebugStringA(("[VERSION] Marcado como aplicado: " + filename + "\n").c_str());
    }

} // namespace autopatch
