#include "grf.h"
#include <zlib.h>
#include <algorithm>
#include <cstring>
#include <Windows.h>

namespace autopatch
{

    // GRF signature
    static const char GRF_SIGNATURE[] = "Master of Magic";

    GrfFile::GrfFile() = default;

    GrfFile::~GrfFile()
    {
        Close();
    }

    bool GrfFile::Open(const std::wstring &path)
    {
        Close();

        m_file.open(path, std::ios::in | std::ios::out | std::ios::binary);
        if (!m_file.is_open())
        {
            // Tenta abrir somente leitura
            m_file.open(path, std::ios::in | std::ios::binary);
            if (!m_file.is_open())
            {
                return false;
            }
        }

        m_path = path;

        if (!ReadHeader())
        {
            Close();
            return false;
        }

        if (!ReadFileTable())
        {
            Close();
            return false;
        }

        m_isOpen = true;
        return true;
    }

    bool GrfFile::Create(const std::wstring &path, GrfVersion version)
    {
        Close();

        m_file.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!m_file.is_open())
        {
            return false;
        }

        m_path = path;

        // Inicializa header
        memset(&m_header, 0, sizeof(m_header));
        memcpy(m_header.signature, GRF_SIGNATURE, sizeof(GRF_SIGNATURE));
        m_header.version = version;
        m_header.fileCount = 0;
        m_header.fileTableOffset = 46; // Tamanho do header

        m_entries.clear();
        m_isOpen = true;
        m_modified = true;

        return true;
    }

    void GrfFile::Close()
    {
        if (m_modified)
        {
            Save();
        }

        if (m_file.is_open())
        {
            m_file.close();
        }

        m_isOpen = false;
        m_modified = false;
        m_entries.clear();
        m_path.clear();
    }

    bool GrfFile::ReadHeader()
    {
        m_file.seekg(0);

        // Lê signature
        m_file.read(m_header.signature, 16);
        if (memcmp(m_header.signature, GRF_SIGNATURE, 15) != 0)
        {
            return false; // Não é um GRF válido
        }

        // Lê encryption key
        m_file.read(reinterpret_cast<char *>(m_header.encryptionKey), 14);

        // Lê file table offset
        m_file.read(reinterpret_cast<char *>(&m_header.fileTableOffset), 4);

        // Lê seed
        m_file.read(reinterpret_cast<char *>(&m_header.seed), 4);

        // Lê file count (real count = stored - seed - 7)
        uint32_t storedCount;
        m_file.read(reinterpret_cast<char *>(&storedCount), 4);

        // Lê version
        uint32_t version;
        m_file.read(reinterpret_cast<char *>(&version), 4);
        m_header.version = static_cast<GrfVersion>(version);

        // Calcula file count real
        m_header.fileCount = storedCount - m_header.seed - 7;

        return m_file.good();
    }

    bool GrfFile::ReadFileTable()
    {
        m_entries.clear();

        // Vai para a tabela de arquivos
        m_file.seekg(m_header.fileTableOffset + 46); // 46 = tamanho do header

        // Lê tamanho comprimido e descomprimido da tabela
        uint32_t compressedSize, uncompressedSize;
        m_file.read(reinterpret_cast<char *>(&compressedSize), 4);
        m_file.read(reinterpret_cast<char *>(&uncompressedSize), 4);

        // Lê dados comprimidos
        std::vector<uint8_t> compressedData(compressedSize);
        m_file.read(reinterpret_cast<char *>(compressedData.data()), compressedSize);

        // Descomprime
        auto tableData = Decompress(compressedData, uncompressedSize);
        if (tableData.empty())
        {
            return false;
        }

        // Parse das entradas
        size_t pos = 0;
        for (uint32_t i = 0; i < m_header.fileCount; i++)
        {
            GrfEntry entry;

            // Lê nome do arquivo (null-terminated)
            entry.filename = reinterpret_cast<const char *>(&tableData[pos]);
            pos += entry.filename.length() + 1;

            // Lê dados da entrada (depende da versão)
            if (m_header.version == GrfVersion::V0x200)
            {
                memcpy(&entry.compressedSize, &tableData[pos], 4);
                pos += 4;
                memcpy(&entry.compressedSizeAligned, &tableData[pos], 4);
                pos += 4;
                memcpy(&entry.uncompressedSize, &tableData[pos], 4);
                pos += 4;
                entry.flags = tableData[pos++];
                memcpy(&entry.offset, &tableData[pos], 4);
                pos += 4;
            }
            else
            {
                // Versões antigas
                memcpy(&entry.compressedSize, &tableData[pos], 4);
                pos += 4;
                memcpy(&entry.compressedSizeAligned, &tableData[pos], 4);
                pos += 4;
                memcpy(&entry.uncompressedSize, &tableData[pos], 4);
                pos += 4;
                entry.flags = tableData[pos++];
                memcpy(&entry.offset, &tableData[pos], 4);
                pos += 4;

                // Calcula cycle para DES
                if (entry.flags & GRFFILE_FLAG_MIXCRYPT)
                {
                    entry.cycle = 1;
                    for (size_t j = 10; entry.compressedSize >= j; j *= 10)
                    {
                        entry.cycle++;
                    }
                }
            }

            m_entries[entry.filename] = entry;
        }

        return true;
    }

    std::vector<std::string> GrfFile::GetFileList() const
    {
        std::vector<std::string> list;
        list.reserve(m_entries.size());

        for (const auto &[name, entry] : m_entries)
        {
            list.push_back(name);
        }

        return list;
    }

    bool GrfFile::FileExists(const std::string &filename) const
    {
        return m_entries.find(filename) != m_entries.end();
    }

    const GrfEntry *GrfFile::GetEntry(const std::string &filename) const
    {
        auto it = m_entries.find(filename);
        if (it != m_entries.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    std::vector<uint8_t> GrfFile::ExtractFile(const std::string &filename)
    {
        auto entry = GetEntry(filename);
        if (!entry || !(entry->flags & GRFFILE_FLAG_FILE))
        {
            return {};
        }

        // Lê dados comprimidos
        m_file.seekg(entry->offset + 46);
        std::vector<uint8_t> compressedData(entry->compressedSizeAligned);
        m_file.read(reinterpret_cast<char *>(compressedData.data()), entry->compressedSizeAligned);

        // Decripta se necessário
        DecryptEntry(compressedData, *entry);

        // Redimensiona para tamanho real
        compressedData.resize(entry->compressedSize);

        // Se não está comprimido, retorna diretamente
        if (entry->compressedSize == entry->uncompressedSize)
        {
            return compressedData;
        }

        // Descomprime
        return Decompress(compressedData, entry->uncompressedSize);
    }

    bool GrfFile::ExtractFileTo(const std::string &filename, const std::wstring &outputPath)
    {
        auto data = ExtractFile(filename);
        if (data.empty())
        {
            return false;
        }

        std::ofstream file(outputPath, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        file.write(reinterpret_cast<const char *>(data.data()), data.size());
        return true;
    }

    bool GrfFile::AddFile(const std::string &filename, const std::vector<uint8_t> &data)
    {
        OutputDebugStringA(("[GRF] AddFile: " + filename + " (" + std::to_string(data.size()) + " bytes)\n").c_str());

        // Comprime os dados
        auto compressed = Compress(data);
        if (compressed.empty() && !data.empty())
        {
            OutputDebugStringA("[GRF] ERRO: Falha na compressão\n");
            return false;
        }

        // Verifica se arquivo já existe
        bool exists = m_entries.find(filename) != m_entries.end();

        // Cria/atualiza entrada
        GrfEntry entry;
        entry.filename = filename;
        entry.uncompressedSize = static_cast<uint32_t>(data.size());
        entry.compressedSize = static_cast<uint32_t>(compressed.size());
        entry.compressedSizeAligned = (entry.compressedSize + 7) & ~7; // Alinha para 8 bytes
        entry.flags = GRFFILE_FLAG_FILE;
        entry.isNew = !exists;
        entry.isModified = exists;
        entry.isDeleted = false;
        entry.cachedData = std::move(compressed);

        // Padding para alinhamento
        entry.cachedData.resize(entry.compressedSizeAligned, 0);

        m_entries[filename] = std::move(entry);
        m_modified = true;

        OutputDebugStringA(("[GRF] Arquivo adicionado: " + filename +
                            " (compressed: " + std::to_string(entry.compressedSize) +
                            ", aligned: " + std::to_string(entry.compressedSizeAligned) + ")\n")
                               .c_str());
        return true;
    }

    bool GrfFile::AddFile(const std::string &filename, const std::wstring &sourcePath)
    {
        std::ifstream file(sourcePath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return false;
        }

        size_t size = file.tellg();
        file.seekg(0);

        std::vector<uint8_t> data(size);
        file.read(reinterpret_cast<char *>(data.data()), size);

        return AddFile(filename, data);
    }

    bool GrfFile::RemoveFile(const std::string &filename)
    {
        auto it = m_entries.find(filename);
        if (it == m_entries.end())
        {
            return false;
        }

        // Marca como deletado em vez de remover
        it->second.isDeleted = true;
        m_modified = true;
        return true;
    }

    bool GrfFile::Save()
    {
        OutputDebugStringA("[GRF] Iniciando Save (QuickMerge)...\n");

        if (!m_isOpen || !m_file.is_open())
        {
            OutputDebugStringA("[GRF] ERRO: Arquivo não está aberto\n");
            return false;
        }

        if (!m_modified)
        {
            OutputDebugStringA("[GRF] Nenhuma modificação pendente\n");
            return true;
        }

        // QuickMerge: Escreve apenas arquivos novos/modificados ao final
        if (!WriteFileData())
        {
            OutputDebugStringA("[GRF] ERRO: Falha ao escrever dados\n");
            return false;
        }

        // Atualiza contagem de arquivos
        uint32_t fileCount = 0;
        for (const auto &[name, entry] : m_entries)
        {
            if (!entry.isDeleted)
            {
                fileCount++;
            }
        }
        m_header.fileCount = fileCount;

        // Escreve tabela de arquivos
        if (!WriteFileTable())
        {
            OutputDebugStringA("[GRF] ERRO: Falha ao escrever tabela de arquivos\n");
            return false;
        }

        // Escreve header atualizado
        if (!WriteHeader())
        {
            OutputDebugStringA("[GRF] ERRO: Falha ao escrever header\n");
            return false;
        }

        // Flush
        m_file.flush();

        m_modified = false;
        OutputDebugStringA(("[GRF] Save concluído com sucesso. Arquivos: " + std::to_string(fileCount) + "\n").c_str());
        return true;
    }

    bool GrfFile::WriteFileData()
    {
        /**
         * QuickMerge Implementation
         *
         * Estratégia:
         * 1. Encontrar onde termina o último arquivo existente (não deletado, não novo)
         * 2. Escrever novos/modificados ao final
         * 3. Atualizar offsets das entradas
         */

        // Passo 1: Encontrar onde termina o último arquivo existente
        uint64_t endOffset = 0;

        for (const auto &[name, entry] : m_entries)
        {
            // Ignorar arquivos deletados ou novos/modificados (serão escritos ao final)
            if (entry.isDeleted || entry.isNew || entry.isModified)
            {
                continue;
            }

            // Calcular onde termina este arquivo
            uint64_t entryEnd = entry.offset + entry.compressedSizeAligned;
            if (entryEnd > endOffset)
            {
                endOffset = entryEnd;
            }
        }

        OutputDebugStringA(("[GRF] Offset final de arquivos existentes: " + std::to_string(endOffset) + "\n").c_str());

        // Passo 2: Escrever arquivos novos/modificados ao final
        uint64_t writeOffset = endOffset;
        int writtenCount = 0;

        for (auto &[name, entry] : m_entries)
        {
            if (entry.isDeleted)
            {
                continue;
            }

            // Apenas processar novos ou modificados
            if (!entry.isNew && !entry.isModified)
            {
                continue;
            }

            if (entry.cachedData.empty())
            {
                OutputDebugStringA(("[GRF] AVISO: Dados vazios para: " + name + "\n").c_str());
                continue;
            }

            // Posicionar e escrever (offset é relativo ao fim do header, que tem 46 bytes)
            m_file.seekp(static_cast<std::streamoff>(46 + writeOffset));
            m_file.write(reinterpret_cast<const char *>(entry.cachedData.data()), entry.cachedData.size());

            if (!m_file)
            {
                OutputDebugStringA(("[GRF] ERRO: Falha ao escrever: " + name + "\n").c_str());
                return false;
            }

            // Atualizar offset da entrada
            entry.offset = static_cast<uint32_t>(writeOffset);
            entry.isNew = false;
            entry.isModified = false;

            OutputDebugStringA(("[GRF] Escrito: " + name + " @ offset " + std::to_string(writeOffset) + "\n").c_str());

            // Avançar offset
            writeOffset += entry.cachedData.size();
            writtenCount++;

            // Limpar cache (dados já foram escritos)
            entry.cachedData.clear();
            entry.cachedData.shrink_to_fit();
        }

        // Atualizar offset da tabela de arquivos
        m_header.fileTableOffset = static_cast<uint32_t>(writeOffset);

        OutputDebugStringA(("[GRF] Escritos " + std::to_string(writtenCount) + " arquivos. Table offset: " +
                            std::to_string(m_header.fileTableOffset) + "\n")
                               .c_str());

        return true;
    }

    bool GrfFile::WriteHeader()
    {
        m_file.seekp(0);

        // Signature (16 bytes)
        m_file.write(m_header.signature, 16);

        // Encryption key (14 bytes)
        m_file.write(reinterpret_cast<const char *>(m_header.encryptionKey), 14);

        // File table offset (4 bytes)
        m_file.write(reinterpret_cast<const char *>(&m_header.fileTableOffset), 4);

        // Seed (4 bytes)
        m_file.write(reinterpret_cast<const char *>(&m_header.seed), 4);

        // File count (stored as count + seed + 7)
        uint32_t storedCount = m_header.fileCount + m_header.seed + 7;
        m_file.write(reinterpret_cast<const char *>(&storedCount), 4);

        // Version (4 bytes)
        uint32_t version = static_cast<uint32_t>(m_header.version);
        m_file.write(reinterpret_cast<const char *>(&version), 4);

        return m_file.good();
    }

    bool GrfFile::WriteFileTable()
    {
        // Constrói tabela de arquivos
        std::vector<uint8_t> tableData;

        for (const auto &[name, entry] : m_entries)
        {
            if (entry.isDeleted)
            {
                continue;
            }

            // Nome do arquivo + null terminator
            tableData.insert(tableData.end(), entry.filename.begin(), entry.filename.end());
            tableData.push_back(0);

            // Compressed size (4 bytes)
            tableData.push_back(entry.compressedSize & 0xFF);
            tableData.push_back((entry.compressedSize >> 8) & 0xFF);
            tableData.push_back((entry.compressedSize >> 16) & 0xFF);
            tableData.push_back((entry.compressedSize >> 24) & 0xFF);

            // Compressed size aligned (4 bytes)
            tableData.push_back(entry.compressedSizeAligned & 0xFF);
            tableData.push_back((entry.compressedSizeAligned >> 8) & 0xFF);
            tableData.push_back((entry.compressedSizeAligned >> 16) & 0xFF);
            tableData.push_back((entry.compressedSizeAligned >> 24) & 0xFF);

            // Uncompressed size (4 bytes)
            tableData.push_back(entry.uncompressedSize & 0xFF);
            tableData.push_back((entry.uncompressedSize >> 8) & 0xFF);
            tableData.push_back((entry.uncompressedSize >> 16) & 0xFF);
            tableData.push_back((entry.uncompressedSize >> 24) & 0xFF);

            // Flags (1 byte)
            tableData.push_back(entry.flags);

            // Offset (4 bytes)
            tableData.push_back(entry.offset & 0xFF);
            tableData.push_back((entry.offset >> 8) & 0xFF);
            tableData.push_back((entry.offset >> 16) & 0xFF);
            tableData.push_back((entry.offset >> 24) & 0xFF);
        }

        OutputDebugStringA(("[GRF] Tabela de arquivos: " + std::to_string(tableData.size()) + " bytes não comprimidos\n").c_str());

        // Comprime tabela
        std::vector<uint8_t> compressedTable = Compress(tableData);

        OutputDebugStringA(("[GRF] Tabela comprimida: " + std::to_string(compressedTable.size()) + " bytes\n").c_str());

        // Posiciona e escreve
        m_file.seekp(static_cast<std::streamoff>(46 + m_header.fileTableOffset));

        uint32_t tableSizeCompressed = static_cast<uint32_t>(compressedTable.size());
        uint32_t tableSize = static_cast<uint32_t>(tableData.size());

        m_file.write(reinterpret_cast<const char *>(&tableSizeCompressed), 4);
        m_file.write(reinterpret_cast<const char *>(&tableSize), 4);
        m_file.write(reinterpret_cast<const char *>(compressedTable.data()), compressedTable.size());

        return m_file.good();
    }

    bool GrfFile::Merge(const GrfFile &other)
    {
        // Copia todas as entradas do outro GRF
        for (const auto &[name, entry] : other.m_entries)
        {
            // TODO: Copiar dados do arquivo
            m_entries[name] = entry;
        }

        m_modified = true;
        return true;
    }

    std::vector<uint8_t> GrfFile::Decompress(const std::vector<uint8_t> &data, size_t uncompressedSize)
    {
        std::vector<uint8_t> result(uncompressedSize);

        uLongf destLen = static_cast<uLongf>(uncompressedSize);
        int ret = uncompress(result.data(), &destLen, data.data(), static_cast<uLong>(data.size()));

        if (ret != Z_OK)
        {
            return {};
        }

        result.resize(destLen);
        return result;
    }

    std::vector<uint8_t> GrfFile::Compress(const std::vector<uint8_t> &data)
    {
        uLongf compressedSize = compressBound(static_cast<uLong>(data.size()));
        std::vector<uint8_t> result(compressedSize);

        int ret = compress(result.data(), &compressedSize, data.data(), static_cast<uLong>(data.size()));

        if (ret != Z_OK)
        {
            return {};
        }

        result.resize(compressedSize);
        return result;
    }

    void GrfFile::DecryptEntry(std::vector<uint8_t> &data, const GrfEntry &entry)
    {
        if (!(entry.flags & (GRFFILE_FLAG_MIXCRYPT | GRFFILE_FLAG_DES)))
        {
            return; // Não está encriptado
        }

        // TODO: Implementar DES decryption para GRF
        // A implementação completa requer as tabelas DES específicas do GRF
    }

    void GrfFile::EncryptEntry(std::vector<uint8_t> &data, GrfEntry &entry)
    {
        // Para GRF 0x200, geralmente não encriptamos
        entry.flags = GRFFILE_FLAG_FILE;
    }

} // namespace autopatch
