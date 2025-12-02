/**
 * @file patching.cpp
 * @brief Implementação do sistema de patching
 * @version 1.0.0
 */

#include "patching.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;

namespace Panic
{

    // =============================================================================
    // FUNÇÕES DE PATCHING PRINCIPAL
    // =============================================================================

    PatchResult ApplyPatch(
        const std::string &thorPath,
        const PatchOptions &options,
        ThorPatchProgressCallback progressCb)
    {
        PatchResult result;

        // Abrir THOR
        ThorArchive thor;
        ThorError err = thor.Open(thorPath);
        if (err != ThorError::OK)
        {
            result.error = PatchError::THOR_NOT_FOUND;
            result.errorMessage = thor.GetLastErrorMessage();
            return result;
        }

        // Determinar modo
        PatchMode mode = options.mode;
        if (mode == PatchMode::AUTO)
        {
            mode = thor.UseGrfMerging() ? PatchMode::GRF_MERGE : PatchMode::DISK_EXTRACT;
        }

        // Aplicar patch
        if (mode == PatchMode::GRF_MERGE)
        {
            // Determinar GRF alvo
            std::string targetGrf = options.targetGrf;
            if (thor.UseGrfMerging() && !thor.GetTargetGrfName().empty())
            {
                targetGrf = thor.GetTargetGrfName();
            }

            // Construir caminho completo
            fs::path grfPath = fs::path(options.extractDirectory) / targetGrf;

            // Verificar se GRF existe
            if (!fs::exists(grfPath))
            {
                result.error = PatchError::GRF_NOT_FOUND;
                result.errorMessage = "GRF not found: " + grfPath.string();
                return result;
            }

            // Criar backup se solicitado
            if (options.createBackup)
            {
                fs::path backupPath = grfPath;
                backupPath += ".backup";

                try
                {
                    fs::copy_file(grfPath, backupPath, fs::copy_options::overwrite_existing);
                }
                catch (const std::exception &e)
                {
                    // Backup falhou, mas podemos continuar
                }
            }

            // Abrir GRF
            GrfFile grf;
            GrfError grfErr = grf.Open(grfPath.string());
            if (grfErr != GrfError::OK)
            {
                result.error = PatchError::OPEN_FAILED;
                result.errorMessage = grf.GetLastErrorMessage();
                return result;
            }

            // Aplicar
            result = ApplyPatchToGrf(thor, grf, progressCb);

            // Salvar GRF
            if (result.error == PatchError::OK)
            {
                grfErr = grf.Save();
                if (grfErr != GrfError::OK)
                {
                    result.error = PatchError::WRITE_FAILED;
                    result.errorMessage = grf.GetLastErrorMessage();
                }
            }
        }
        else
        {
            // Modo extração para disco
            result = ApplyPatchToDisk(thor, options.extractDirectory, progressCb);
        }

        return result;
    }

    PatchResult ApplyPatchToGrf(
        ThorArchive &thor,
        GrfFile &grf,
        ThorPatchProgressCallback progressCb)
    {
        PatchResult result;

        const auto &entries = thor.GetEntries();
        size_t total = entries.size();
        size_t current = 0;

        for (const auto &entry : entries)
        {
            current++;

            // Callback de progresso
            if (progressCb)
            {
                if (!progressCb("patching", current, total, entry.relativePath))
                {
                    result.error = PatchError::CANCELLED;
                    result.errorMessage = "Cancelled by user";
                    return result;
                }
            }

            // Pular arquivo de integridade
            if (entry.relativePath == "data.integrity")
            {
                continue;
            }

            // Se arquivo marcado para remoção
            if (entry.isRemoved)
            {
                GrfError err = grf.RemoveFile(entry.relativePath);
                if (err == GrfError::OK)
                {
                    result.filesRemoved++;
                }
                continue;
            }

            // Ler dados do arquivo
            auto data = thor.ReadFileContent(entry.relativePath);
            if (data.empty() && entry.size > 0)
            {
                // Erro ao ler
                continue;
            }

            // Adicionar ao GRF
            GrfError err = grf.AddFile(entry.relativePath, data, true);
            if (err == GrfError::OK)
            {
                result.filesAdded++;
                result.bytesWritten += data.size();
            }
        }

        return result;
    }

    PatchResult ApplyPatchToDisk(
        ThorArchive &thor,
        const std::string &outputDir,
        ThorPatchProgressCallback progressCb)
    {
        PatchResult result;

        const auto &entries = thor.GetEntries();
        size_t total = entries.size();
        size_t current = 0;

        fs::path basePath(outputDir);

        for (const auto &entry : entries)
        {
            current++;

            // Callback de progresso
            if (progressCb)
            {
                if (!progressCb("extracting", current, total, entry.relativePath))
                {
                    result.error = PatchError::CANCELLED;
                    result.errorMessage = "Cancelled by user";
                    return result;
                }
            }

            // Pular arquivo de integridade
            if (entry.relativePath == "data.integrity")
            {
                continue;
            }

            // Construir caminho de saída
            fs::path outputPath = basePath / entry.relativePath;

            // Se arquivo marcado para remoção
            if (entry.isRemoved)
            {
                if (fs::exists(outputPath))
                {
                    try
                    {
                        fs::remove(outputPath);
                        result.filesRemoved++;
                    }
                    catch (...)
                    {
                        // Ignorar erro de remoção
                    }
                }
                continue;
            }

            // Ler dados do arquivo
            auto data = thor.ReadFileContent(entry.relativePath);
            if (data.empty() && entry.size > 0)
            {
                // Erro ao ler
                continue;
            }

            // Criar diretórios pai
            try
            {
                fs::create_directories(outputPath.parent_path());
            }
            catch (...)
            {
                continue;
            }

            // Escrever arquivo
            std::ofstream outFile(outputPath, std::ios::binary);
            if (!outFile.is_open())
            {
                continue;
            }

            outFile.write(reinterpret_cast<const char *>(data.data()), data.size());
            outFile.close();

            result.filesAdded++;
            result.bytesWritten += data.size();
        }

        return result;
    }

    PatchResult ApplyPatches(
        const std::vector<std::string> &thorPaths,
        const PatchOptions &options,
        ThorPatchProgressCallback progressCb)
    {
        PatchResult totalResult;
        size_t patchNum = 0;
        size_t totalPatches = thorPaths.size();

        for (const auto &thorPath : thorPaths)
        {
            patchNum++;

            // Callback de progresso com informação do patch
            ThorPatchProgressCallback wrappedCb = nullptr;
            if (progressCb)
            {
                wrappedCb = [&](const std::string &stage, size_t current, size_t total, const std::string &filename) -> bool
                {
                    std::string fullStage = "[" + std::to_string(patchNum) + "/" + std::to_string(totalPatches) + "] " + stage;
                    return progressCb(fullStage, current, total, filename);
                };
            }

            // Aplicar patch
            PatchResult result = ApplyPatch(thorPath, options, wrappedCb);

            // Agregar resultado
            totalResult.filesAdded += result.filesAdded;
            totalResult.filesRemoved += result.filesRemoved;
            totalResult.bytesWritten += result.bytesWritten;

            // Verificar erro
            if (result.error != PatchError::OK)
            {
                if (!options.continueOnError)
                {
                    totalResult.error = result.error;
                    totalResult.errorMessage = result.errorMessage;
                    return totalResult;
                }
            }
        }

        return totalResult;
    }

    // =============================================================================
    // FUNÇÕES DE UTILITÁRIO
    // =============================================================================

    size_t ReadLocalVersion(const std::string &directory)
    {
        fs::path versionFile = fs::path(directory) / PatchConstants::LOCAL_VERSION_FILE;

        std::ifstream file(versionFile);
        if (!file.is_open())
        {
            return 0;
        }

        size_t version = 0;
        file >> version;
        return version;
    }

    bool SaveLocalVersion(const std::string &directory, size_t version)
    {
        fs::path versionFile = fs::path(directory) / PatchConstants::LOCAL_VERSION_FILE;

        std::ofstream file(versionFile);
        if (!file.is_open())
        {
            return false;
        }

        file << version;
        return true;
    }

    std::vector<ThorPatchInfo> GetPatchesToApply(
        const ThorPatchList &patchList,
        size_t localVersion)
    {
        std::vector<ThorPatchInfo> result;

        for (const auto &patch : patchList)
        {
            if (patch.index > localVersion)
            {
                result.push_back(patch);
            }
        }

        // Garantir ordenação
        std::sort(result.begin(), result.end(),
                  [](const ThorPatchInfo &a, const ThorPatchInfo &b)
                  {
                      return a.index < b.index;
                  });

        return result;
    }

    bool ValidateThorFile(const std::string &thorPath)
    {
        ThorArchive thor;
        ThorError err = thor.Open(thorPath);
        if (err != ThorError::OK)
        {
            return false;
        }

        return thor.IsValid();
    }

    ThorInfo GetThorInfo(const std::string &thorPath)
    {
        ThorInfo info;

        ThorArchive thor;
        ThorError err = thor.Open(thorPath);
        if (err != ThorError::OK)
        {
            info.valid = false;
            info.errorMessage = thor.GetLastErrorMessage();
            return info;
        }

        info.valid = true;
        info.useGrfMerging = thor.UseGrfMerging();
        info.targetGrfName = thor.GetTargetGrfName();
        info.fileCount = static_cast<uint32_t>(thor.GetFileCount());

        return info;
    }

} // namespace Panic
