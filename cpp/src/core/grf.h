#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <fstream>

namespace autopatch
{

    // Versões GRF suportadas
    enum class GrfVersion : uint32_t
    {
        V0x101 = 0x101,
        V0x102 = 0x102,
        V0x103 = 0x103,
        V0x200 = 0x200
    };

    // Flags de entrada GRF
    enum GrfEntryFlags : uint8_t
    {
        GRFFILE_FLAG_FILE = 0x01,
        GRFFILE_FLAG_MIXCRYPT = 0x02,
        GRFFILE_FLAG_DES = 0x04
    };

    // Entrada de arquivo no GRF
    struct GrfEntry
    {
        std::string filename;               // Nome do arquivo (em encoding coreano)
        uint32_t compressedSize = 0;        // Tamanho comprimido
        uint32_t compressedSizeAligned = 0; // Tamanho alinhado
        uint32_t uncompressedSize = 0;      // Tamanho original
        uint32_t offset = 0;                // Offset no arquivo GRF
        uint8_t flags = 0;                  // Flags
        uint32_t cycle = 0;                 // Cycle para DES

        // Para arquivos novos/modificados
        bool isNew = false;              // Arquivo novo (não existe no GRF original)
        bool isModified = false;         // Arquivo modificado
        bool isDeleted = false;          // Arquivo marcado para deleção
        std::vector<uint8_t> cachedData; // Dados comprimidos em cache (para novos/modificados)
    };

    // Header do arquivo GRF
    struct GrfHeader
    {
        char signature[16];        // "Master of Magic\0"
        uint8_t encryptionKey[14]; // Chave de encriptação
        uint32_t fileTableOffset;  // Offset da tabela de arquivos
        uint32_t seed;             // Seed
        uint32_t fileCount;        // Número de arquivos
        GrfVersion version;        // Versão
    };

    // Classe para leitura/escrita de arquivos GRF
    class GrfFile
    {
    public:
        GrfFile();
        ~GrfFile();

        // Abre um arquivo GRF existente
        bool Open(const std::wstring &path);

        // Cria um novo arquivo GRF
        bool Create(const std::wstring &path, GrfVersion version = GrfVersion::V0x200);

        // Fecha o arquivo
        void Close();

        // Verifica se está aberto
        bool IsOpen() const { return m_isOpen; }

        // Obtém a versão
        GrfVersion GetVersion() const { return m_header.version; }

        // Obtém número de arquivos
        size_t GetFileCount() const { return m_entries.size(); }

        // Lista todos os arquivos
        std::vector<std::string> GetFileList() const;

        // Verifica se um arquivo existe
        bool FileExists(const std::string &filename) const;

        // Obtém informações de um arquivo
        const GrfEntry *GetEntry(const std::string &filename) const;

        // Extrai um arquivo para memória
        std::vector<uint8_t> ExtractFile(const std::string &filename);

        // Extrai um arquivo para disco
        bool ExtractFileTo(const std::string &filename, const std::wstring &outputPath);

        // Adiciona/substitui um arquivo
        bool AddFile(const std::string &filename, const std::vector<uint8_t> &data);
        bool AddFile(const std::string &filename, const std::wstring &sourcePath);

        // Remove um arquivo
        bool RemoveFile(const std::string &filename);

        // Salva alterações (repack)
        bool Save();

        // Mescla outro GRF neste (para patching)
        bool Merge(const GrfFile &other);

    private:
        bool ReadHeader();
        bool ReadFileTable();
        bool WriteHeader();
        bool WriteFileTable();
        bool WriteFileData(); // QuickMerge - escreve apenas novos/modificados

        std::vector<uint8_t> Decompress(const std::vector<uint8_t> &data, size_t uncompressedSize);
        std::vector<uint8_t> Compress(const std::vector<uint8_t> &data);
        void DecryptEntry(std::vector<uint8_t> &data, const GrfEntry &entry);
        void EncryptEntry(std::vector<uint8_t> &data, GrfEntry &entry);

        std::wstring m_path;
        std::fstream m_file;
        bool m_isOpen = false;
        bool m_modified = false;
        GrfHeader m_header = {};
        std::map<std::string, GrfEntry> m_entries;
    };

} // namespace autopatch
