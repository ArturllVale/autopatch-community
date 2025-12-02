/**
 * @file thor_archive.cpp
 * @brief Implementação da classe ThorArchive
 * @version 1.0.0
 */

#include "thor_archive.hpp"
#include "../compression/compression.hpp"

#include <algorithm>
#include <sstream>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

namespace Panic
{

    // =============================================================================
    // FUNÇÕES UTILITÁRIAS
    // =============================================================================

    ThorPatchList ParsePatchList(const std::string &content)
    {
        ThorPatchList patchList;
        std::istringstream stream(content);
        std::string line;

        while (std::getline(stream, line))
        {
            // Remove espaços no início e fim
            size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos)
                continue;
            line = line.substr(start);

            size_t end = line.find_last_not_of(" \t\r\n");
            if (end != std::string::npos)
                line = line.substr(0, end + 1);

            // Ignora linhas vazias e comentários
            if (line.empty() || line[0] == '/' || line[0] == '#')
                continue;

            // Formato: índice nome_arquivo.thor
            std::istringstream lineStream(line);
            size_t index;
            std::string fileName;

            if (lineStream >> index >> fileName)
            {
                ThorPatchInfo info;
                info.index = index;
                info.fileName = fileName;
                patchList.push_back(info);
            }
        }

        // Ordena por índice
        std::sort(patchList.begin(), patchList.end(),
                  [](const ThorPatchInfo &a, const ThorPatchInfo &b)
                  {
                      return a.index < b.index;
                  });

        return patchList;
    }

    // =============================================================================
    // THOR ARCHIVE - CONSTRUTORES
    // =============================================================================

    ThorArchive::~ThorArchive()
    {
        Close();
    }

    ThorArchive::ThorArchive(ThorArchive &&other) noexcept
        : m_filePath(std::move(other.m_filePath)),
          m_fileStream(std::move(other.m_fileStream)),
          m_isOpen(other.m_isOpen),
          m_header(std::move(other.m_header)),
          m_entriesList(std::move(other.m_entriesList)),
          m_entriesMap(std::move(other.m_entriesMap)),
          m_checksums(std::move(other.m_checksums)),
          m_hasChecksums(other.m_hasChecksums),
          m_lastError(other.m_lastError),
          m_lastErrorMessage(std::move(other.m_lastErrorMessage))
    {
        other.m_isOpen = false;
    }

    ThorArchive &ThorArchive::operator=(ThorArchive &&other) noexcept
    {
        if (this != &other)
        {
            Close();
            m_filePath = std::move(other.m_filePath);
            m_fileStream = std::move(other.m_fileStream);
            m_isOpen = other.m_isOpen;
            m_header = std::move(other.m_header);
            m_entriesList = std::move(other.m_entriesList);
            m_entriesMap = std::move(other.m_entriesMap);
            m_checksums = std::move(other.m_checksums);
            m_hasChecksums = other.m_hasChecksums;
            m_lastError = other.m_lastError;
            m_lastErrorMessage = std::move(other.m_lastErrorMessage);
            other.m_isOpen = false;
        }
        return *this;
    }

    // =============================================================================
    // OPERAÇÕES DE ARQUIVO
    // =============================================================================

    ThorError ThorArchive::Open(const std::string &filepath)
    {
        Close();

        // Verificar se arquivo existe
        if (!fs::exists(filepath))
        {
            SetError(ThorError::FILE_NOT_FOUND, "File not found: " + filepath);
            return m_lastError;
        }

        // Abrir arquivo
        m_fileStream.open(filepath, std::ios::binary);
        if (!m_fileStream.is_open())
        {
            SetError(ThorError::OPEN_FAILED, "Failed to open file: " + filepath);
            return m_lastError;
        }

        m_filePath = filepath;

        // Ler header
        ThorError err = ReadHeader();
        if (err != ThorError::OK)
        {
            Close();
            return err;
        }

        // Ler tabela de arquivos
        err = ReadFileTable();
        if (err != ThorError::OK)
        {
            Close();
            return err;
        }

        // Parsear informações de integridade se disponíveis
        ParseDataIntegrity();

        m_isOpen = true;
        return ThorError::OK;
    }

    void ThorArchive::Close()
    {
        if (m_fileStream.is_open())
        {
            m_fileStream.close();
        }

        m_filePath.clear();
        m_isOpen = false;
        m_header = ThorHeader();
        m_entriesList.clear();
        m_entriesMap.clear();
        m_checksums.clear();
        m_hasChecksums = false;
        m_lastError = ThorError::OK;
        m_lastErrorMessage.clear();
    }

    // =============================================================================
    // ACESSO A ARQUIVOS
    // =============================================================================

    const ThorFileEntry *ThorArchive::GetFileEntry(const std::string &filename) const
    {
        std::string normalized = NormalizeFilename(filename);
        auto it = m_entriesMap.find(normalized);
        if (it != m_entriesMap.end())
        {
            return &m_entriesList[it->second];
        }
        return nullptr;
    }

    bool ThorArchive::ContainsFile(const std::string &filename) const
    {
        return GetFileEntry(filename) != nullptr;
    }

    std::vector<uint8_t> ThorArchive::ReadFileContent(const std::string &filename)
    {
        const ThorFileEntry *entry = GetFileEntry(filename);
        if (!entry)
        {
            SetError(ThorError::ENTRY_NOT_FOUND, "Entry not found: " + filename);
            return {};
        }

        // Se arquivo marcado para remoção, retorna vazio
        if (entry->isRemoved)
        {
            return {};
        }

        // Se tamanho zero, retorna vazio
        if (entry->size == 0)
        {
            return {};
        }

        // Ler dados comprimidos
        std::vector<uint8_t> compressedData(entry->sizeCompressed);
        m_fileStream.seekg(entry->offset);
        m_fileStream.read(reinterpret_cast<char *>(compressedData.data()), entry->sizeCompressed);

        if (!m_fileStream)
        {
            SetError(ThorError::READ_FAILED, "Failed to read file data: " + filename);
            return {};
        }

        // Descomprimir
        std::vector<uint8_t> decompressedData = Compression::Decompress(compressedData, entry->size);
        if (decompressedData.empty() && entry->size > 0)
        {
            SetError(ThorError::DECOMPRESS_FAILED, "Failed to decompress: " + filename);
            return {};
        }

        return decompressedData;
    }

    std::vector<uint8_t> ThorArchive::GetEntryRawData(const std::string &filename)
    {
        const ThorFileEntry *entry = GetFileEntry(filename);
        if (!entry)
        {
            SetError(ThorError::ENTRY_NOT_FOUND, "Entry not found: " + filename);
            return {};
        }

        if (entry->isRemoved || entry->sizeCompressed == 0)
        {
            return {};
        }

        std::vector<uint8_t> data(entry->sizeCompressed);
        m_fileStream.seekg(entry->offset);
        m_fileStream.read(reinterpret_cast<char *>(data.data()), entry->sizeCompressed);

        if (!m_fileStream)
        {
            SetError(ThorError::READ_FAILED, "Failed to read raw data: " + filename);
            return {};
        }

        return data;
    }

    ThorError ThorArchive::ExtractFile(const std::string &filename, const std::string &destinationPath)
    {
        auto data = ReadFileContent(filename);
        if (data.empty() && GetLastError() != ThorError::OK)
        {
            return GetLastError();
        }

        // Criar diretórios pai
        fs::path destPath(destinationPath);
        fs::create_directories(destPath.parent_path());

        // Escrever arquivo
        std::ofstream outFile(destinationPath, std::ios::binary);
        if (!outFile.is_open())
        {
            SetError(ThorError::OPEN_FAILED, "Failed to create output file: " + destinationPath);
            return m_lastError;
        }

        outFile.write(reinterpret_cast<const char *>(data.data()), data.size());
        return ThorError::OK;
    }

    // =============================================================================
    // VALIDAÇÃO
    // =============================================================================

    ThorError ThorArchive::Validate()
    {
        if (!m_isOpen)
        {
            SetError(ThorError::OPEN_FAILED, "Archive not open");
            return m_lastError;
        }

        // Se não tem checksums, não há como validar
        if (!m_hasChecksums)
        {
            return ThorError::OK;
        }

        // Verificar cada arquivo com checksum
        for (const auto &entry : m_entriesList)
        {
            if (entry.isRemoved)
                continue;

            auto checksumIt = m_checksums.find(NormalizeFilename(entry.relativePath));
            if (checksumIt != m_checksums.end())
            {
                // Ler dados e calcular CRC32
                auto rawData = GetEntryRawData(entry.relativePath);
                if (rawData.empty() && entry.sizeCompressed > 0)
                {
                    continue; // Erro de leitura, já reportado
                }

                // Calcular CRC32
                uint32_t crc = 0;
                // TODO: Implementar CRC32 se necessário

                if (crc != checksumIt->second)
                {
                    SetError(ThorError::INTEGRITY_FAILED,
                             "Checksum mismatch for: " + entry.relativePath);
                    return m_lastError;
                }
            }
        }

        return ThorError::OK;
    }

    bool ThorArchive::IsValid()
    {
        return Validate() == ThorError::OK;
    }

    // =============================================================================
    // MÉTODOS PRIVADOS - LEITURA
    // =============================================================================

    ThorError ThorArchive::ReadHeader()
    {
        // Primeiro, ler 48 bytes para verificar qual formato é
        char magic[ThorConstants::MAGIC_LEGACY_SIZE];
        m_fileStream.read(magic, ThorConstants::MAGIC_LEGACY_SIZE);

        // Verificar formato GRF Editor (24 bytes)
        if (std::strncmp(magic, ThorConstants::MAGIC, ThorConstants::MAGIC_SIZE) == 0)
        {
            // Voltar para posição correta (após os 24 bytes do magic)
            m_fileStream.seekg(ThorConstants::MAGIC_SIZE);
        }
        // Verificar formato Thor Patcher legado (48 bytes)
        else if (std::strncmp(magic, ThorConstants::MAGIC_LEGACY, ThorConstants::MAGIC_LEGACY_SIZE) == 0)
        {
            // Já estamos na posição correta (após os 48 bytes)
        }
        else
        {
            SetError(ThorError::INVALID_MAGIC, "Invalid THOR magic");
            return m_lastError;
        }

        if (!m_fileStream)
        {
            SetError(ThorError::INVALID_MAGIC, "Failed to read THOR magic");
            return m_lastError;
        }

        // Ler flags
        uint8_t useGrfMerging;
        m_fileStream.read(reinterpret_cast<char *>(&useGrfMerging), 1);
        m_header.useGrfMerging = (useGrfMerging == 1);

        // Ler file count
        m_fileStream.read(reinterpret_cast<char *>(&m_header.fileCount), 4);

        // Ler mode
        m_fileStream.read(reinterpret_cast<char *>(&m_header.mode), 2);

        // Validar mode
        if (m_header.mode != ThorConstants::MODE_SINGLE_FILE &&
            m_header.mode != ThorConstants::MODE_MULTIPLE_FILES)
        {
            SetError(ThorError::INVALID_MODE, "Invalid THOR mode: " + std::to_string(m_header.mode));
            return m_lastError;
        }

        // Ler target GRF name
        uint8_t targetGrfNameSize;
        m_fileStream.read(reinterpret_cast<char *>(&targetGrfNameSize), 1);

        if (targetGrfNameSize > 0)
        {
            m_header.targetGrfName.resize(targetGrfNameSize);
            m_fileStream.read(&m_header.targetGrfName[0], targetGrfNameSize);
        }

        if (!m_fileStream)
        {
            SetError(ThorError::CORRUPT_HEADER, "Failed to read THOR header");
            return m_lastError;
        }

        return ThorError::OK;
    }

    ThorError ThorArchive::ReadFileTable()
    {
        if (m_header.mode == ThorConstants::MODE_SINGLE_FILE)
        {
            return ReadSingleFileTable();
        }
        else
        {
            return ReadMultipleFilesTable();
        }
    }

    ThorError ThorArchive::ReadSingleFileTable()
    {
        // Modo arquivo único:
        // - file_table_offset: 8 bytes
        // - Depois do offset, os dados do arquivo

        uint64_t fileTableOffset;
        m_fileStream.read(reinterpret_cast<char *>(&fileTableOffset), 8);

        if (!m_fileStream)
        {
            SetError(ThorError::CORRUPT_FILE_TABLE, "Failed to read single file table offset");
            return m_lastError;
        }

        // Ir para a tabela
        m_fileStream.seekg(fileTableOffset);

        // Ler entrada única
        ThorFileEntry entry;

        // Ler tamanho do nome
        uint8_t nameSize;
        m_fileStream.read(reinterpret_cast<char *>(&nameSize), 1);

        entry.relativePath.resize(nameSize);
        m_fileStream.read(&entry.relativePath[0], nameSize);

        // Ler flags
        uint8_t flags;
        m_fileStream.read(reinterpret_cast<char *>(&flags), 1);
        entry.isRemoved = (flags & ThorConstants::ENTRY_FLAG_REMOVE) != 0;

        // Ler offset dos dados (relativo ao início do arquivo)
        m_fileStream.read(reinterpret_cast<char *>(&entry.offset), 8);

        // Ler tamanho comprimido
        m_fileStream.read(reinterpret_cast<char *>(&entry.sizeCompressed), 4);

        // Ler tamanho descomprimido
        m_fileStream.read(reinterpret_cast<char *>(&entry.size), 4);

        if (!m_fileStream)
        {
            SetError(ThorError::CORRUPT_FILE_TABLE, "Failed to read single file entry");
            return m_lastError;
        }

        // Adicionar à lista
        m_entriesList.push_back(entry);
        m_entriesMap[NormalizeFilename(entry.relativePath)] = 0;

        return ThorError::OK;
    }

    ThorError ThorArchive::ReadMultipleFilesTable()
    {
        // Modo múltiplos arquivos (GRF Editor format):
        // - file_table_compressed_size: 4 bytes
        // - file_table_offset: 4 bytes (NOT 8!)
        // - Tabela comprimida com zlib (.NET DeflateStream)

        uint32_t tableCompressedSize;
        m_fileStream.read(reinterpret_cast<char *>(&tableCompressedSize), 4);

        uint32_t fileTableOffset; // 4 bytes, não 8!
        m_fileStream.read(reinterpret_cast<char *>(&fileTableOffset), 4);

        if (!m_fileStream)
        {
            SetError(ThorError::CORRUPT_FILE_TABLE, "Failed to read multiple files table header");
            return m_lastError;
        }

        // Ir para a tabela
        m_fileStream.seekg(fileTableOffset);

        // Ler tabela comprimida
        std::vector<uint8_t> compressedTable(tableCompressedSize);
        m_fileStream.read(reinterpret_cast<char *>(compressedTable.data()), tableCompressedSize);

        if (!m_fileStream)
        {
            SetError(ThorError::CORRUPT_FILE_TABLE, "Failed to read compressed file table");
            return m_lastError;
        }

        // Descomprimir tabela - tentar DeflateStream primeiro, depois zlib
        size_t estimatedSize = tableCompressedSize * 10;
        std::vector<uint8_t> table = Compression::DecompressDeflate(compressedTable, estimatedSize);

        if (table.empty() && tableCompressedSize > 0)
        {
            // Tentar com Decompress normal (zlib)
            table = Compression::Decompress(compressedTable, estimatedSize);
        }

        if (table.empty() && tableCompressedSize > 0)
        {
            SetError(ThorError::DECOMPRESS_FAILED, "Failed to decompress file table");
            return m_lastError;
        }

        // Parsear entradas
        // Formato GRF Editor:
        // - nameSize: 1 byte
        // - name: nameSize bytes
        // - flags: 1 byte (0x01 = remove)
        // Se não remove:
        //   - offset: 4 bytes (relativo ao dataOffset)
        //   - sizeCompressed: 4 bytes
        //   - sizeDecompressed: 4 bytes

        size_t pos = 0;
        for (uint32_t i = 0; i < m_header.fileCount; i++)
        {
            if (pos >= table.size())
                break;

            ThorFileEntry entry;

            // Ler tamanho do nome
            uint8_t nameSize = table[pos++];
            if (pos + nameSize > table.size())
                break;

            entry.relativePath.assign(reinterpret_cast<char *>(&table[pos]), nameSize);
            pos += nameSize;

            // Ler flags
            if (pos >= table.size())
                break;
            uint8_t flags = table[pos++];
            entry.isRemoved = (flags & ThorConstants::ENTRY_FLAG_REMOVE) != 0;

            if (!entry.isRemoved)
            {
                // Ler offset (4 bytes, não 8!)
                if (pos + 4 > table.size())
                    break;
                uint32_t relOffset;
                std::memcpy(&relOffset, &table[pos], 4);
                entry.offset = relOffset;
                pos += 4;

                // Ler tamanho comprimido (4 bytes)
                if (pos + 4 > table.size())
                    break;
                std::memcpy(&entry.sizeCompressed, &table[pos], 4);
                pos += 4;

                // Ler tamanho descomprimido (4 bytes)
                if (pos + 4 > table.size())
                    break;
                std::memcpy(&entry.size, &table[pos], 4);
                pos += 4;
            }
            else
            {
                entry.offset = 0;
                entry.sizeCompressed = 0;
                entry.size = 0;
            }

            // Adicionar à lista
            m_entriesMap[NormalizeFilename(entry.relativePath)] = m_entriesList.size();
            m_entriesList.push_back(entry);
        }

        return ThorError::OK;
    }

    void ThorArchive::ParseDataIntegrity()
    {
        // Procurar arquivo "data.integrity"
        const ThorFileEntry *integrityEntry = GetFileEntry("data.integrity");
        if (!integrityEntry || integrityEntry->isRemoved)
        {
            return;
        }

        // Ler conteúdo
        auto data = ReadFileContent("data.integrity");
        if (data.empty())
        {
            return;
        }

        // Parsear formato:
        // arquivo=checksum
        // arquivo=checksum
        std::string content(data.begin(), data.end());
        std::istringstream stream(content);
        std::string line;

        while (std::getline(stream, line))
        {
            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos)
                continue;

            std::string filename = line.substr(0, eqPos);
            std::string checksumStr = line.substr(eqPos + 1);

            // Remover espaços
            filename.erase(0, filename.find_first_not_of(" \t"));
            filename.erase(filename.find_last_not_of(" \t\r\n") + 1);
            checksumStr.erase(0, checksumStr.find_first_not_of(" \t"));
            checksumStr.erase(checksumStr.find_last_not_of(" \t\r\n") + 1);

            if (!filename.empty() && !checksumStr.empty())
            {
                try
                {
                    uint32_t checksum = static_cast<uint32_t>(std::stoul(checksumStr));
                    m_checksums[NormalizeFilename(filename)] = checksum;
                }
                catch (...)
                {
                    // Ignorar erros de parse
                }
            }
        }

        m_hasChecksums = !m_checksums.empty();
    }

    std::string ThorArchive::NormalizeFilename(const std::string &filename)
    {
        std::string result = filename;

        // Converter para lowercase
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);

        // Converter / para backslash
        std::replace(result.begin(), result.end(), '/', '\\');

        return result;
    }

    void ThorArchive::SetError(ThorError error, const std::string &message)
    {
        m_lastError = error;
        m_lastErrorMessage = message.empty() ? ThorErrorToString(error) : message;
    }

} // namespace Panic
