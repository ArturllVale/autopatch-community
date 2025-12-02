/**
 * @file patching.hpp
 * @brief Sistema de aplicação de patches THOR em GRF
 * @version 1.0.0
 *
 * Este módulo implementa a aplicação de patches THOR em arquivos GRF,
 * compatível com o formato usado pelo rpatchur.
 *
 * Suporta dois métodos de patching:
 * - GRF Merge: Arquivos são adicionados diretamente ao GRF
 * - Disk Extract: Arquivos são extraídos para o disco (RO moderno)
 */

#ifndef PANIC_PATCHING_HPP
#define PANIC_PATCHING_HPP

#include "thor_archive.hpp"
#include "grf_file.hpp"

#include <string>
#include <vector>
#include <functional>
#include <filesystem>

namespace Panic
{

    // =============================================================================
    // CONSTANTES
    // =============================================================================

    namespace PatchConstants
    {
        /// Arquivo de registro de versão local
        constexpr const char *LOCAL_VERSION_FILE = "plist.version";

        /// Arquivo temporário durante patching
        constexpr const char *TEMP_SUFFIX = ".patching";
    }

    // =============================================================================
    // TIPOS E ESTRUTURAS
    // =============================================================================

    /**
     * @brief Códigos de erro do sistema de patching
     */
    enum class PatchError
    {
        OK = 0,

        // Erros de arquivo
        GRF_NOT_FOUND,
        THOR_NOT_FOUND,
        OPEN_FAILED,
        WRITE_FAILED,

        // Erros de patching
        INVALID_PATCH,
        CORRUPT_PATCH,
        TARGET_MISMATCH,
        INTEGRITY_FAILED,

        // Erros de rede
        DOWNLOAD_FAILED,

        // Outros
        CANCELLED,
        UNKNOWN_ERROR
    };

    /**
     * @brief Converte PatchError para string
     */
    inline const char *PatchErrorToString(PatchError err)
    {
        switch (err)
        {
        case PatchError::OK:
            return "Success";
        case PatchError::GRF_NOT_FOUND:
            return "GRF file not found";
        case PatchError::THOR_NOT_FOUND:
            return "THOR file not found";
        case PatchError::OPEN_FAILED:
            return "Failed to open file";
        case PatchError::WRITE_FAILED:
            return "Failed to write file";
        case PatchError::INVALID_PATCH:
            return "Invalid patch file";
        case PatchError::CORRUPT_PATCH:
            return "Corrupt patch file";
        case PatchError::TARGET_MISMATCH:
            return "Target GRF mismatch";
        case PatchError::INTEGRITY_FAILED:
            return "Integrity check failed";
        case PatchError::DOWNLOAD_FAILED:
            return "Download failed";
        case PatchError::CANCELLED:
            return "Operation cancelled";
        default:
            return "Unknown error";
        }
    }

    /**
     * @brief Modo de aplicação do patch
     */
    enum class PatchMode
    {
        /// Adiciona arquivos ao GRF (método clássico)
        GRF_MERGE,

        /// Extrai arquivos para o disco (RO moderno)
        DISK_EXTRACT,

        /// Detecta automaticamente baseado no THOR
        AUTO
    };

    /**
     * @brief Resultado de um patch aplicado
     */
    struct PatchResult
    {
        PatchError error = PatchError::OK;
        std::string errorMessage;

        size_t filesAdded = 0;
        size_t filesRemoved = 0;
        size_t bytesWritten = 0;
    };

    /**
     * @brief Opções para aplicação de patch
     */
    struct PatchOptions
    {
        /// Modo de aplicação
        PatchMode mode = PatchMode::AUTO;

        /// Diretório de extração (para DISK_EXTRACT)
        std::string extractDirectory = ".";

        /// GRF alvo (para GRF_MERGE)
        std::string targetGrf = "data.grf";

        /// Criar backup do GRF antes de modificar
        bool createBackup = false;

        /// Verificar integridade após aplicar
        bool verifyAfterPatch = false;

        /// Continuar mesmo se alguns arquivos falharem
        bool continueOnError = false;
    };

    /**
     * @brief Callback de progresso do patching
     * @param stage Estágio atual (reading, extracting, writing, etc.)
     * @param current Item atual
     * @param total Total de itens
     * @param filename Nome do arquivo sendo processado
     * @return true para continuar, false para cancelar
     */
    using ThorPatchProgressCallback = std::function<bool(
        const std::string &stage,
        size_t current,
        size_t total,
        const std::string &filename)>;

    // =============================================================================
    // FUNÇÕES DE PATCHING
    // =============================================================================

    /**
     * @brief Aplica um arquivo THOR a um GRF
     *
     * @param thorPath Caminho do arquivo .thor
     * @param options Opções de patching
     * @param progressCb Callback de progresso
     * @return Resultado do patching
     *
     * @note Se o THOR especifica useGrfMerging=true, aplica ao GRF.
     *       Caso contrário, extrai para o disco.
     */
    PatchResult ApplyPatch(
        const std::string &thorPath,
        const PatchOptions &options = {},
        ThorPatchProgressCallback progressCb = nullptr);

    /**
     * @brief Aplica um arquivo THOR diretamente a um GRF aberto
     *
     * @param thor Arquivo THOR aberto
     * @param grf Arquivo GRF aberto para escrita
     * @param progressCb Callback de progresso
     * @return Resultado do patching
     */
    PatchResult ApplyPatchToGrf(
        ThorArchive &thor,
        GrfFile &grf,
        ThorPatchProgressCallback progressCb = nullptr);

    /**
     * @brief Extrai arquivos de um THOR para o disco
     *
     * @param thor Arquivo THOR aberto
     * @param outputDir Diretório de saída
     * @param progressCb Callback de progresso
     * @return Resultado do patching
     */
    PatchResult ApplyPatchToDisk(
        ThorArchive &thor,
        const std::string &outputDir,
        ThorPatchProgressCallback progressCb = nullptr);

    /**
     * @brief Aplica múltiplos patches em sequência
     *
     * @param thorPaths Lista de caminhos de arquivos .thor (em ordem)
     * @param options Opções de patching
     * @param progressCb Callback de progresso
     * @return Resultado agregado
     */
    PatchResult ApplyPatches(
        const std::vector<std::string> &thorPaths,
        const PatchOptions &options = {},
        ThorPatchProgressCallback progressCb = nullptr);

    // =============================================================================
    // FUNÇÕES DE UTILITÁRIO
    // =============================================================================

    /**
     * @brief Lê a versão local instalada
     *
     * @param directory Diretório do cliente
     * @return Índice da última versão instalada, ou 0 se não houver
     */
    size_t ReadLocalVersion(const std::string &directory);

    /**
     * @brief Salva a versão local
     *
     * @param directory Diretório do cliente
     * @param version Índice da versão
     * @return true se salvou com sucesso
     */
    bool SaveLocalVersion(const std::string &directory, size_t version);

    /**
     * @brief Calcula patches necessários
     *
     * @param patchList Lista de patches do servidor
     * @param localVersion Versão local atual
     * @return Lista de patches a baixar (ordenados por índice)
     */
    std::vector<ThorPatchInfo> GetPatchesToApply(
        const ThorPatchList &patchList,
        size_t localVersion);

    /**
     * @brief Valida um arquivo THOR
     *
     * @param thorPath Caminho do arquivo
     * @return true se é um THOR válido
     */
    bool ValidateThorFile(const std::string &thorPath);

    /**
     * @brief Obtém informações de um arquivo THOR sem extrair
     *
     * @param thorPath Caminho do arquivo
     * @return Estrutura com informações (fileCount, targetGrf, etc.)
     */
    struct ThorInfo
    {
        bool valid = false;
        bool useGrfMerging = false;
        std::string targetGrfName;
        uint32_t fileCount = 0;
        std::string errorMessage;
    };
    ThorInfo GetThorInfo(const std::string &thorPath);

} // namespace Panic

#endif // PANIC_PATCHING_HPP
