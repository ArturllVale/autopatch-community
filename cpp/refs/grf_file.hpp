/**
 * @file grf_file.hpp
 * @brief Classe C++ para manipulação completa de arquivos GRF
 * @version 2.0.0
 *
 * Suporta:
 * - Leitura e escrita de GRF (versões 0x102, 0x103, 0x200, 0x300)
 * - DES encryption/decryption
 * - Custom encryption key (256 bytes)
 * - Compressão ZLIB
 * - Criação de novos GRFs
 *
 * Baseado no GRFEditor do Tokei (https://github.com/Tokeiburu/GRFEditor)
 */

#ifndef PANIC_GRF_FILE_HPP
#define PANIC_GRF_FILE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <fstream>
#include <cstdint>
#include <mutex>
#include <optional>
#include <filesystem>
#include <array>

namespace Panic
{

    // Forward declarations
    class DESCrypto;
    class Compression;

    // =============================================================================
    // CONSTANTES
    // =============================================================================

    /// Constantes do formato GRF
    namespace GrfConstants
    {
        constexpr size_t HEADER_SIZE = 46;
        constexpr const char *MAGIC = "Master of Magic";
        constexpr size_t MAGIC_SIZE = 16;
        constexpr size_t KEY_SIZE = 14;

        // Versões
        constexpr uint32_t VERSION_102 = 0x102; // Alpha GRF (DES encryption)
        constexpr uint32_t VERSION_103 = 0x103; // Alpha GRF (DES encryption)
        constexpr uint32_t VERSION_200 = 0x200; // Standard GRF
        constexpr uint32_t VERSION_300 = 0x300; // Large GRF (>4GB support)

        // Flags de entrada
        constexpr uint8_t FLAG_FILE = 0x01;
        constexpr uint8_t FLAG_ENCRYPT_MIXED = 0x02;
        constexpr uint8_t FLAG_ENCRYPT_HEADER = 0x04;
        constexpr uint8_t FLAG_ADDED = 0x08; // Adicionado pelo autopatcher
    }

    // =============================================================================
    // TIPOS E ESTRUTURAS
    // =============================================================================

    /**
     * @brief Códigos de erro
     */
    enum class GrfError
    {
        OK = 0,
        FILE_NOT_FOUND,
        OPEN_FAILED,
        INVALID_MAGIC,
        UNSUPPORTED_VERSION,
        CORRUPT_HEADER,
        CORRUPT_FILE_TABLE,
        DECOMPRESS_FAILED,
        COMPRESS_FAILED,
        OUT_OF_MEMORY,
        FILE_LOCKED,
        READ_FAILED,
        WRITE_FAILED,
        ENTRY_NOT_FOUND,
        INVALID_OPERATION
    };

    /**
     * @brief Converte código de erro para string
     */
    inline const char *GrfErrorToString(GrfError err)
    {
        switch (err)
        {
        case GrfError::OK:
            return "Success";
        case GrfError::FILE_NOT_FOUND:
            return "File not found";
        case GrfError::OPEN_FAILED:
            return "Failed to open file";
        case GrfError::INVALID_MAGIC:
            return "Invalid GRF magic";
        case GrfError::UNSUPPORTED_VERSION:
            return "Unsupported GRF version";
        case GrfError::CORRUPT_HEADER:
            return "Corrupt header";
        case GrfError::CORRUPT_FILE_TABLE:
            return "Corrupt file table";
        case GrfError::DECOMPRESS_FAILED:
            return "Decompression failed";
        case GrfError::COMPRESS_FAILED:
            return "Compression failed";
        case GrfError::OUT_OF_MEMORY:
            return "Out of memory";
        case GrfError::FILE_LOCKED:
            return "File is locked";
        case GrfError::READ_FAILED:
            return "Read failed";
        case GrfError::WRITE_FAILED:
            return "Write failed";
        case GrfError::ENTRY_NOT_FOUND:
            return "Entry not found";
        case GrfError::INVALID_OPERATION:
            return "Invalid operation";
        default:
            return "Unknown error";
        }
    }

    /**
     * @brief Entrada de arquivo no GRF
     */
    struct GrfEntry
    {
        std::string filename; // Nome do arquivo (caminho relativo)

        uint32_t sizeCompressed = 0;        // Tamanho comprimido
        uint32_t sizeCompressedAligned = 0; // Tamanho comprimido alinhado
        uint32_t sizeDecompressed = 0;      // Tamanho descomprimido

        uint64_t offset = 0; // Offset dos dados no GRF
        uint8_t flags = 0;   // Flags (tipo, criptografia)
        int32_t cycle = -1;  // Cycle para DES encryption

        // Status para escrita
        bool isModified = false; // Foi modificado
        bool isNew = false;      // É novo arquivo
        bool isDeleted = false;  // Marcado para deleção

        // Cache de dados (para arquivos modificados/novos)
        std::vector<uint8_t> cachedData;

        /**
         * @brief Verifica se é um arquivo (não diretório)
         */
        bool IsFile() const { return (flags & GrfConstants::FLAG_FILE) != 0; }

        /**
         * @brief Verifica se usa criptografia DES mista
         */
        bool IsEncryptedMixed() const { return (flags & GrfConstants::FLAG_ENCRYPT_MIXED) != 0; }

        /**
         * @brief Verifica se usa criptografia DES no header
         */
        bool IsEncryptedHeader() const { return (flags & GrfConstants::FLAG_ENCRYPT_HEADER) != 0; }

        /**
         * @brief Verifica se está criptografado
         */
        bool IsEncrypted() const { return IsEncryptedMixed() || IsEncryptedHeader(); }
    };

    /**
     * @brief Header do GRF
     */
    struct GrfHeader
    {
        char magic[GrfConstants::MAGIC_SIZE];
        char key[GrfConstants::KEY_SIZE];
        uint64_t fileTableOffset = 0;
        int32_t seed = 0;
        int32_t rawFileCount = 0;
        int32_t realFileCount = 0;
        uint32_t version = GrfConstants::VERSION_200;
    };

    /**
     * @brief Callback de progresso para GRF
     * @param current Item atual
     * @param total Total de itens
     * @param message Mensagem de status
     * @return true para continuar, false para cancelar
     * @note Renomeado para evitar conflito com http_client.hpp
     */
    using GrfProgressCallback = std::function<bool(int current, int total, const std::string &message)>;

    // =============================================================================
    // CLASSE PRINCIPAL
    // =============================================================================

    /**
     * @brief Classe para manipulação de arquivos GRF
     *
     * Exemplo de uso:
     * @code
     * Panic::GrfFile grf;
     *
     * // Abrir GRF existente
     * if (grf.Open("data.grf") == Panic::GrfError::OK) {
     *     // Ler arquivo
     *     auto data = grf.ExtractFile("data\\sprite\\test.spr");
     *
     *     // Adicionar arquivo
     *     grf.AddFile("data\\test.txt", myData);
     *
     *     // Salvar mudanças
     *     grf.Save();
     * }
     *
     * // Criar novo GRF
     * Panic::GrfFile newGrf;
     * newGrf.Create("new.grf");
     * newGrf.AddFile("test.txt", data);
     * newGrf.Save();
     * @endcode
     */
    class GrfFile
    {
    public:
        // =========================================================================
        // CONSTRUTORES E DESTRUTOR
        // =========================================================================

        GrfFile() = default;
        ~GrfFile();

        // Non-copyable, movable
        GrfFile(const GrfFile &) = delete;
        GrfFile &operator=(const GrfFile &) = delete;
        GrfFile(GrfFile &&) noexcept;
        GrfFile &operator=(GrfFile &&) noexcept;

        // =========================================================================
        // OPERAÇÕES DE ARQUIVO
        // =========================================================================

        /**
         * @brief Abre um arquivo GRF existente
         * @param filepath Caminho do arquivo
         * @param progressCb Callback de progresso (opcional)
         * @return Código de erro
         */
        GrfError Open(const std::string &filepath, GrfProgressCallback progressCb = nullptr);

        /**
         * @brief Cria um novo arquivo GRF
         * @param filepath Caminho do arquivo
         * @param version Versão do GRF (padrão: 0x200)
         * @return Código de erro
         */
        GrfError Create(const std::string &filepath, uint32_t version = GrfConstants::VERSION_200);

        /**
         * @brief Fecha o arquivo GRF
         */
        void Close();

        /**
         * @brief Salva alterações no GRF (tenta QuickMerge primeiro, fallback para FullRepack)
         * @param progressCb Callback de progresso (opcional)
         * @return Código de erro
         */
        GrfError Save(GrfProgressCallback progressCb = nullptr);

        /**
         * @brief Salva como novo arquivo (sempre faz FullRepack)
         * @param filepath Novo caminho
         * @param progressCb Callback de progresso (opcional)
         * @return Código de erro
         */
        GrfError SaveAs(const std::string &filepath, GrfProgressCallback progressCb = nullptr);

        /**
         * @brief QuickMerge - Apenas adiciona novos/modificados ao final (rápido)
         * @param progressCb Callback de progresso (opcional)
         * @return Código de erro
         */
        GrfError QuickSave(GrfProgressCallback progressCb = nullptr);

        /**
         * @brief FullRepack - Recria a GRF do zero (lento, mas seguro)
         * @param progressCb Callback de progresso (opcional)
         * @param outputPath Caminho de saída (vazio = mesmo arquivo)
         * @return Código de erro
         */
        GrfError FullRepack(GrfProgressCallback progressCb = nullptr, const std::string &outputPath = "");

        /**
         * @brief Verifica se o GRF está aberto
         */
        bool IsOpen() const { return m_isOpen; }

        /**
         * @brief Obtém o caminho do arquivo
         */
        const std::string &GetFilePath() const { return m_filePath; }

        // =========================================================================
        // MANIPULAÇÃO DE ENTRADAS
        // =========================================================================

        /**
         * @brief Obtém número de arquivos no GRF
         */
        size_t GetFileCount() const { return m_entries.size(); }

        /**
         * @brief Procura entrada por nome
         * @param filename Nome do arquivo (case insensitive, converte '/' para '\\')
         * @return Ponteiro para entrada ou nullptr
         */
        GrfEntry *FindEntry(const std::string &filename);
        const GrfEntry *FindEntry(const std::string &filename) const;

        /**
         * @brief Verifica se arquivo existe no GRF
         */
        bool FileExists(const std::string &filename) const;

        /**
         * @brief Obtém lista de todos os arquivos
         */
        std::vector<std::string> GetFileList() const;

        /**
         * @brief Obtém lista de arquivos por filtro
         * @param filter Filtro (ex: "*.spr", "data\\sprite\\*")
         */
        std::vector<std::string> GetFileList(const std::string &filter) const;

        /**
         * @brief Itera sobre todas as entradas
         */
        template <typename Func>
        void ForEachEntry(Func &&func)
        {
            for (auto &pair : m_entries)
            {
                func(pair.second);
            }
        }

        // =========================================================================
        // EXTRAÇÃO DE ARQUIVOS
        // =========================================================================

        /**
         * @brief Extrai dados de um arquivo
         * @param filename Nome do arquivo
         * @return Dados extraídos ou vetor vazio em erro
         */
        std::vector<uint8_t> ExtractFile(const std::string &filename);

        /**
         * @brief Extrai arquivo para disco
         * @param filename Nome do arquivo no GRF
         * @param outputPath Caminho de destino
         * @return Código de erro
         */
        GrfError ExtractToFile(const std::string &filename, const std::string &outputPath);

        /**
         * @brief Extrai todos os arquivos para diretório
         * @param outputDir Diretório de destino
         * @param progressCb Callback de progresso
         * @return Código de erro
         */
        GrfError ExtractAll(const std::string &outputDir, GrfProgressCallback progressCb = nullptr);

        // =========================================================================
        // ADIÇÃO/MODIFICAÇÃO DE ARQUIVOS
        // =========================================================================

        /**
         * @brief Adiciona ou substitui arquivo
         * @param filename Nome do arquivo no GRF
         * @param data Dados do arquivo
         * @param compress Comprimir dados (padrão: true)
         * @return Código de erro
         */
        GrfError AddFile(const std::string &filename, const std::vector<uint8_t> &data,
                         bool compress = true);

        /**
         * @brief Adiciona arquivo a partir de disco
         * @param grfPath Nome no GRF
         * @param diskPath Caminho no disco
         * @param compress Comprimir dados
         * @return Código de erro
         */
        GrfError AddFileFromDisk(const std::string &grfPath, const std::string &diskPath,
                                 bool compress = true);

        /**
         * @brief Adiciona diretório recursivamente
         * @param basePath Caminho base no GRF
         * @param diskDir Diretório no disco
         * @param progressCb Callback de progresso
         * @return Código de erro
         */
        GrfError AddDirectory(const std::string &basePath, const std::string &diskDir,
                              GrfProgressCallback progressCb = nullptr);

        /**
         * @brief Remove arquivo do GRF
         * @param filename Nome do arquivo
         * @return Código de erro
         * @note Renomeado de DeleteFile para evitar conflito com macro Windows
         */
        GrfError RemoveFile(const std::string &filename);

        /**
         * @brief Renomeia arquivo no GRF
         * @param oldName Nome antigo
         * @param newName Novo nome
         * @return Código de erro
         */
        GrfError RenameFile(const std::string &oldName, const std::string &newName);

        // =========================================================================
        // CRIPTOGRAFIA
        // =========================================================================

        /**
         * @brief Define chave de criptografia customizada (256 bytes)
         * @param key Chave de 256 bytes
         */
        void SetEncryptionKey(const std::array<uint8_t, 256> &key);

        /**
         * @brief Remove chave de criptografia
         */
        void ClearEncryptionKey();

        /**
         * @brief Verifica se tem chave de criptografia
         */
        bool HasEncryptionKey() const { return m_hasCustomKey; }

        /**
         * @brief Obtém versão do GRF
         */
        uint32_t GetVersion() const { return m_header.version; }

        // =========================================================================
        // INFORMAÇÕES E DIAGNÓSTICO
        // =========================================================================

        /**
         * @brief Obtém último erro
         */
        GrfError GetLastError() const { return m_lastError; }

        /**
         * @brief Obtém mensagem do último erro
         */
        const std::string &GetLastErrorMessage() const { return m_lastErrorMessage; }

        /**
         * @brief Verifica integridade do GRF
         * @param progressCb Callback de progresso
         * @return Lista de arquivos com problemas
         */
        std::vector<std::string> VerifyIntegrity(GrfProgressCallback progressCb = nullptr);

        /**
         * @brief Calcula hash MD5 de arquivo
         * @param filename Nome do arquivo
         * @return Hash MD5 em hex ou string vazia em erro
         */
        std::string CalculateFileMD5(const std::string &filename);

        /**
         * @brief Obtém estatísticas do GRF
         */
        struct Statistics
        {
            size_t totalFiles = 0;
            size_t totalSize = 0;
            size_t compressedSize = 0;
            size_t encryptedFiles = 0;
        };
        Statistics GetStatistics() const;

    private:
        // =========================================================================
        // MEMBROS PRIVADOS
        // =========================================================================

        std::string m_filePath;
        std::fstream m_fileStream;
        bool m_isOpen = false;
        bool m_isModified = false;

        GrfHeader m_header;
        std::unordered_map<std::string, GrfEntry> m_entries;

        // Criptografia customizada
        bool m_hasCustomKey = false;
        std::array<uint8_t, 256> m_customKey;

        // Thread safety
        mutable std::mutex m_mutex;

        // Error handling
        GrfError m_lastError = GrfError::OK;
        std::string m_lastErrorMessage;

        // =========================================================================
        // MÉTODOS PRIVADOS
        // =========================================================================

        /**
         * @brief Lê header do GRF
         */
        GrfError ReadHeader();

        /**
         * @brief Lê tabela de arquivos
         */
        GrfError ReadFileTable(GrfProgressCallback progressCb);

        /**
         * @brief Lê tabela de arquivos versão 1.x (0x102, 0x103)
         */
        GrfError ReadFileTableV1(const std::vector<uint8_t> &tableData, GrfProgressCallback progressCb);

        /**
         * @brief Lê tabela de arquivos versão 2.x (0x200)
         */
        GrfError ReadFileTableV2(const std::vector<uint8_t> &tableData, GrfProgressCallback progressCb);

        /**
         * @brief Escreve header do GRF
         */
        GrfError WriteHeader();

        /**
         * @brief Escreve tabela de arquivos
         */
        GrfError WriteFileTable();

        /**
         * @brief Escreve dados dos arquivos modificados
         */
        GrfError WriteFileData(GrfProgressCallback progressCb);

        /**
         * @brief Normaliza nome de arquivo (lowercase, backslash)
         */
        static std::string NormalizeFilename(const std::string &filename);

        /**
         * @brief Verifica se nome corresponde ao filtro
         */
        static bool MatchesFilter(const std::string &filename, const std::string &filter);

        /**
         * @brief Descriptografa dados de entrada
         */
        void DecryptEntry(GrfEntry &entry, std::vector<uint8_t> &data);

        /**
         * @brief Criptografa dados para entrada
         */
        void EncryptEntry(GrfEntry &entry, std::vector<uint8_t> &data);

        /**
         * @brief Define erro
         */
        void SetError(GrfError error, const std::string &message = "");
    };

} // namespace Panic

#endif // PANIC_GRF_FILE_HPP
