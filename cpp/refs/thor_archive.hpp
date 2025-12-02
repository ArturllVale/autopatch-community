/**
 * @file thor_archive.hpp
 * @brief Classe para leitura de arquivos THOR (patches de GRF)
 * @version 1.0.0
 *
 * Formato THOR baseado no rpatchur/gruf:
 * - Header: magic + flags + file_count + mode + target_grf
 * - File table: lista de arquivos com offsets e tamanhos
 * - Data: dados comprimidos com zlib
 * - Checksums: opcional, para validação de integridade
 *
 * Referência: https://github.com/L1nkZ/rpatchur/tree/main/gruf
 */

#ifndef PANIC_THOR_ARCHIVE_HPP
#define PANIC_THOR_ARCHIVE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <cstdint>
#include <functional>

namespace Panic
{

    // Forward declarations
    class GrfFile;

    // =============================================================================
    // CONSTANTES
    // =============================================================================

    namespace ThorConstants
    {
        /// Magic do arquivo THOR (formato GRF Editor / Tokeiburu)
        /// O formato original usava "ASSF (C) 2007 Aeokan (aeokan@gmail.com)" (48 bytes)
        /// O formato GRF Editor usa "ASSF (C) 2007 Aeomin DEV" (24 bytes)
        constexpr const char *MAGIC = "ASSF (C) 2007 Aeomin DEV";
        constexpr size_t MAGIC_SIZE = 24;

        /// Magic legado (Thor Patcher original)
        constexpr const char *MAGIC_LEGACY = "ASSF (C) 2007 Aeokan (aeokan@gmail.com)";
        constexpr size_t MAGIC_LEGACY_SIZE = 48;

        /// Header size
        constexpr size_t HEADER_BASE_SIZE = 36; // magic(24) + use_grf_merging(1) + file_count(4) + mode(2) + target_grf_name_size(1) + reserved(4)

        /// Modos do THOR
        constexpr int16_t MODE_INVALID = 0;
        constexpr int16_t MODE_SINGLE_FILE = 33;    // Patch de arquivo único
        constexpr int16_t MODE_MULTIPLE_FILES = 48; // Patch de múltiplos arquivos

        /// Flags de entrada
        constexpr uint8_t ENTRY_FLAG_REMOVE = 0x01; // Arquivo deve ser removido
    }

    // =============================================================================
    // TIPOS E ESTRUTURAS
    // =============================================================================

    /**
     * @brief Códigos de erro do THOR
     */
    enum class ThorError
    {
        OK = 0,
        FILE_NOT_FOUND,
        OPEN_FAILED,
        INVALID_MAGIC,
        INVALID_MODE,
        CORRUPT_HEADER,
        CORRUPT_FILE_TABLE,
        DECOMPRESS_FAILED,
        ENTRY_NOT_FOUND,
        INTEGRITY_FAILED,
        READ_FAILED
    };

    /**
     * @brief Converte código de erro para string
     */
    inline const char *ThorErrorToString(ThorError err)
    {
        switch (err)
        {
        case ThorError::OK:
            return "Success";
        case ThorError::FILE_NOT_FOUND:
            return "File not found";
        case ThorError::OPEN_FAILED:
            return "Failed to open file";
        case ThorError::INVALID_MAGIC:
            return "Invalid THOR magic";
        case ThorError::INVALID_MODE:
            return "Invalid THOR mode";
        case ThorError::CORRUPT_HEADER:
            return "Corrupt header";
        case ThorError::CORRUPT_FILE_TABLE:
            return "Corrupt file table";
        case ThorError::DECOMPRESS_FAILED:
            return "Decompression failed";
        case ThorError::ENTRY_NOT_FOUND:
            return "Entry not found";
        case ThorError::INTEGRITY_FAILED:
            return "Integrity check failed";
        case ThorError::READ_FAILED:
            return "Read failed";
        default:
            return "Unknown error";
        }
    }

    /**
     * @brief Entrada de arquivo no THOR
     */
    struct ThorFileEntry
    {
        std::string relativePath;    // Caminho relativo no GRF/disco
        uint32_t sizeCompressed = 0; // Tamanho comprimido
        uint32_t size = 0;           // Tamanho descomprimido
        uint64_t offset = 0;         // Offset dos dados no arquivo THOR
        bool isRemoved = false;      // Se deve remover o arquivo (não adicionar)

        // Checksum opcional (CRC32)
        uint32_t checksum = 0;
        bool hasChecksum = false;
    };

    /**
     * @brief Header do THOR
     */
    struct ThorHeader
    {
        bool useGrfMerging = true; // true = GRF, false = disco
        uint32_t fileCount = 0;    // Número de arquivos
        int16_t mode = 0;          // Modo (single/multiple)
        std::string targetGrfName; // Nome do GRF alvo (vazio = default)
    };

    /**
     * @brief Informação de patch do plist.txt
     */
    struct ThorPatchInfo
    {
        size_t index = 0;     // Índice do patch
        std::string fileName; // Nome do arquivo .thor
    };

    /**
     * @brief Lista de patches (plist.txt)
     */
    using ThorPatchList = std::vector<ThorPatchInfo>;

    /**
     * @brief Callback de progresso para THOR
     */
    using ThorProgressCallback = std::function<bool(int current, int total, const std::string &message)>;

    // =============================================================================
    // FUNÇÕES UTILITÁRIAS
    // =============================================================================

    /**
     * @brief Parseia conteúdo do plist.txt
     * @param content Conteúdo do arquivo
     * @return Lista de patches ordenada por índice
     *
     * Formato do plist.txt:
     * ```
     * //869 iteminfo_20170423.thor    <- Comentário (ignorado)
     * 870 iteminfo_20170423_.thor
     * 871 sprites_20170427.thor
     * ```
     */
    ThorPatchList ParsePatchList(const std::string &content);

    // =============================================================================
    // CLASSE PRINCIPAL - LEITURA
    // =============================================================================

    /**
     * @brief Classe para leitura de arquivos THOR
     *
     * Exemplo de uso:
     * @code
     * Panic::ThorArchive thor;
     * if (thor.Open("patch.thor") == Panic::ThorError::OK) {
     *     // Verificar destino
     *     if (thor.UseGrfMerging()) {
     *         std::cout << "Patch para GRF: " << thor.GetTargetGrfName() << std::endl;
     *     } else {
     *         std::cout << "Patch para disco" << std::endl;
     *     }
     *
     *     // Listar arquivos
     *     for (const auto& entry : thor.GetEntries()) {
     *         std::cout << entry.relativePath << std::endl;
     *     }
     *
     *     // Extrair arquivo
     *     auto data = thor.ReadFileContent("data/sprite/test.spr");
     * }
     * @endcode
     */
    class ThorArchive
    {
    public:
        // =========================================================================
        // CONSTRUTORES E DESTRUTOR
        // =========================================================================

        ThorArchive() = default;
        ~ThorArchive();

        // Non-copyable
        ThorArchive(const ThorArchive &) = delete;
        ThorArchive &operator=(const ThorArchive &) = delete;

        // Movable
        ThorArchive(ThorArchive &&) noexcept;
        ThorArchive &operator=(ThorArchive &&) noexcept;

        // =========================================================================
        // OPERAÇÕES DE ARQUIVO
        // =========================================================================

        /**
         * @brief Abre arquivo THOR
         * @param filepath Caminho do arquivo
         * @return Código de erro
         */
        ThorError Open(const std::string &filepath);

        /**
         * @brief Fecha arquivo THOR
         */
        void Close();

        /**
         * @brief Verifica se está aberto
         */
        bool IsOpen() const { return m_isOpen; }

        /**
         * @brief Obtém caminho do arquivo
         */
        const std::string &GetFilePath() const { return m_filePath; }

        // =========================================================================
        // INFORMAÇÕES DO THOR
        // =========================================================================

        /**
         * @brief Verifica se patch é para GRF (true) ou disco (false)
         */
        bool UseGrfMerging() const { return m_header.useGrfMerging; }

        /**
         * @brief Obtém número de arquivos no patch
         */
        size_t GetFileCount() const { return m_entriesList.size(); }

        /**
         * @brief Obtém nome do GRF alvo
         * @return Nome do GRF ou string vazia para default
         */
        const std::string &GetTargetGrfName() const { return m_header.targetGrfName; }

        /**
         * @brief Obtém header
         */
        const ThorHeader &GetHeader() const { return m_header; }

        // =========================================================================
        // ACESSO A ARQUIVOS
        // =========================================================================

        /**
         * @brief Obtém todas as entradas
         */
        const std::vector<ThorFileEntry> &GetEntries() const { return m_entriesList; }

        /**
         * @brief Procura entrada por nome
         * @param filename Nome do arquivo
         * @return Ponteiro para entrada ou nullptr
         */
        const ThorFileEntry *GetFileEntry(const std::string &filename) const;

        /**
         * @brief Verifica se arquivo existe
         */
        bool ContainsFile(const std::string &filename) const;

        /**
         * @brief Lê conteúdo de arquivo (descomprimido)
         * @param filename Nome do arquivo
         * @return Dados ou vetor vazio em erro
         */
        std::vector<uint8_t> ReadFileContent(const std::string &filename);

        /**
         * @brief Lê dados raw de arquivo (comprimido)
         * @param filename Nome do arquivo
         * @return Dados ou vetor vazio em erro
         */
        std::vector<uint8_t> GetEntryRawData(const std::string &filename);

        /**
         * @brief Extrai arquivo para disco
         * @param filename Nome do arquivo no THOR
         * @param destinationPath Caminho de destino
         * @return Código de erro
         */
        ThorError ExtractFile(const std::string &filename, const std::string &destinationPath);

        // =========================================================================
        // VALIDAÇÃO
        // =========================================================================

        /**
         * @brief Verifica integridade do arquivo THOR
         * @return OK se válido, INTEGRITY_FAILED se corrompido
         *
         * Verifica checksums internos se disponíveis
         */
        ThorError Validate();

        /**
         * @brief Verifica se arquivo THOR é válido (checksum)
         * @return true se válido
         */
        bool IsValid();

        // =========================================================================
        // ERRO
        // =========================================================================

        /**
         * @brief Obtém último erro
         */
        ThorError GetLastError() const { return m_lastError; }

        /**
         * @brief Obtém mensagem do último erro
         */
        const std::string &GetLastErrorMessage() const { return m_lastErrorMessage; }

    private:
        // =========================================================================
        // MEMBROS PRIVADOS
        // =========================================================================

        std::string m_filePath;
        std::ifstream m_fileStream;
        bool m_isOpen = false;

        ThorHeader m_header;
        std::vector<ThorFileEntry> m_entriesList;
        std::unordered_map<std::string, size_t> m_entriesMap; // filename -> index

        // Checksums para validação (se disponíveis)
        std::unordered_map<std::string, uint32_t> m_checksums;
        bool m_hasChecksums = false;

        // Error handling
        ThorError m_lastError = ThorError::OK;
        std::string m_lastErrorMessage;

        // =========================================================================
        // MÉTODOS PRIVADOS
        // =========================================================================

        /**
         * @brief Lê header do THOR
         */
        ThorError ReadHeader();

        /**
         * @brief Lê tabela de arquivos
         */
        ThorError ReadFileTable();

        /**
         * @brief Lê tabela de arquivo único (MODE_SINGLE_FILE)
         */
        ThorError ReadSingleFileTable();

        /**
         * @brief Lê tabela de múltiplos arquivos (MODE_MULTIPLE_FILES)
         */
        ThorError ReadMultipleFilesTable();

        /**
         * @brief Parseia informações de integridade (data.integrity)
         */
        void ParseDataIntegrity();

        /**
         * @brief Normaliza nome de arquivo
         */
        static std::string NormalizeFilename(const std::string &filename);

        /**
         * @brief Define erro
         */
        void SetError(ThorError error, const std::string &message = "");
    };

} // namespace Panic

#endif // PANIC_THOR_ARCHIVE_HPP
