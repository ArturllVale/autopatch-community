#pragma once

#include "config.h"
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <thread>
#include <atomic>

namespace autopatch
{

    // Tipo de destino do patch
    enum class PatchTarget
    {
        GRF,    // Insere no GRF
        Folder, // Extrai para pasta do cliente
        Both    // GRF e pasta
    };

    // Informações de um patch
    struct PatchInfo
    {
        int index = 0;
        std::string filename;
        std::string url;
        uint64_t size = 0;
        std::string checksum;
        std::string targetGrf;                    // GRF alvo (se aplicável)
        std::string targetFolder;                 // Pasta alvo (se aplicável)
        PatchTarget target = PatchTarget::Folder; // Destino padrão: pasta
        bool extract = true;                      // Extrair arquivo? (para arquivos comprimidos)
        bool downloaded = false;                  // Já foi baixado?
    };

    // Status do patcher
    enum class PatcherStatus
    {
        Idle,
        CheckingUpdates,
        Downloading,
        Patching,
        Complete,
        Error
    };

    // Callback de progresso
    using PatchProgressCallback = std::function<void(PatcherStatus status,
                                                     const std::wstring &message,
                                                     float progress)>;

    // Classe principal do patcher
    class Patcher
    {
    public:
        Patcher();
        ~Patcher();

        // Inicializa com configuração
        bool Initialize(const PatcherConfig &config);

        // Verifica atualizações
        void CheckForUpdates();

        // Aplica patches pendentes
        void ApplyPatches();

        // Cancela operação atual
        void Cancel();

        // Obtém status atual
        PatcherStatus GetStatus() const { return m_status; }

        // Define callback de progresso
        void SetProgressCallback(PatchProgressCallback callback);

        // Verifica se está ocupado
        bool IsBusy() const;

        // Lista de patches pendentes
        const std::vector<PatchInfo> &GetPendingPatches() const { return m_pendingPatches; }

        // Inicia o jogo
        bool StartGame();

    private:
        void WorkerThread();
        void DownloadPatchList();
        void DownloadPatch(const PatchInfo &patch);
        void ApplyPatch(const PatchInfo &patch);
        bool ApplyThorPatch(const std::wstring &tempPath, const PatchInfo &patch);
        bool ApplyRgzPatch(const std::wstring &tempPath, const PatchInfo &patch);
        bool ApplyGpfPatch(const std::wstring &tempPath, const PatchInfo &patch);
        bool MergeGrfPatch(const std::wstring &tempPath, const PatchInfo &patch);
        bool CopyPatchToFolder(const std::wstring &tempPath, const PatchInfo &patch);
        void CreateDirectoryRecursive(const std::wstring &path);
        void ReportProgress(PatcherStatus status, const std::wstring &message, float progress);

        // Version tracking
        void LoadAppliedPatches();
        void SaveAppliedPatches();
        bool IsPatchApplied(const std::string &filename) const;
        void MarkPatchApplied(const std::string &filename);
        std::wstring GetVersionFilePath() const;

        std::set<std::string> m_appliedPatches; // Set de patches já aplicados

        std::string m_patchListUrl;
        std::string m_clientExe;
        std::string m_clientArgs;
        std::vector<std::string> m_grfFiles;

        std::vector<PatchInfo> m_pendingPatches;

        std::atomic<PatcherStatus> m_status{PatcherStatus::Idle};
        std::atomic<bool> m_cancelRequested{false};

        PatchProgressCallback m_progressCallback;

        std::thread m_workerThread;
    };

} // namespace autopatch
