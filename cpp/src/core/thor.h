#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <fstream>

namespace autopatch
{

    class GrfFile;

    // Modos do arquivo THOR
    enum class ThorMode : uint8_t
    {
        Invalid = 0,
        SingleFile = 0x21, // '!'
        MultiFile = 0x30   // '0'
    };

    // Entrada de arquivo THOR
    struct ThorEntry
    {
        std::string filename;
        uint8_t flags; // 1=add/update, 2=delete
        uint32_t offset;
        uint32_t compressedSize;
        uint32_t uncompressedSize;
    };

    // Classe para leitura de arquivos THOR
    class ThorFile
    {
    public:
        ThorFile();
        ~ThorFile();

        // Abre arquivo THOR
        bool Open(const std::wstring &path);

        // Fecha arquivo
        void Close();

        // Verifica se está aberto
        bool IsOpen() const { return m_isOpen; }

        // Obtém modo
        ThorMode GetMode() const { return m_mode; }

        // Obtém GRF alvo (para SingleFile)
        const std::string &GetTargetGrf() const { return m_targetGrf; }

        // Verifica se deve fazer merge com GRF (true) ou extrair para disco (false)
        bool UseGrfMerging() const { return m_useGrfMerging; }

        // Lista de entradas
        const std::vector<ThorEntry> &GetEntries() const { return m_entries; }

        // Extrai arquivo para memória
        std::vector<uint8_t> ExtractFile(const ThorEntry &entry);

        // Aplica patch a um GRF (merge)
        bool ApplyTo(GrfFile &grf);

        // Aplica patch extraindo para disco
        bool ApplyToDisk(const std::wstring &outputDir);

    private:
        bool ReadHeader();
        bool ReadFileTable();
        bool ReadSingleFileTable();
        bool ReadMultipleFilesTable();

        std::wstring m_path;
        std::ifstream m_file;
        bool m_isOpen = false;

        ThorMode m_mode = ThorMode::Invalid;
        bool m_useGrfMerging = true;
        std::string m_targetGrf;
        uint32_t m_fileCount = 0;
        uint32_t m_fileTableOffset = 0;
        uint32_t m_fileTableCompLen = 0;
        uint32_t m_dataStartOffset = 0;
        std::vector<ThorEntry> m_entries;
    };

} // namespace autopatch
