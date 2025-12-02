/**
 * @file grf_file.cpp
 * @brief Implementação da classe GrfFile
 * @version 2.0.0
 */

#include "grf_file.hpp"
#include "../crypto/des_crypto.hpp"
#include "../compression/compression.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <regex>

// Windows para MD5
#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")
#endif

namespace Panic
{

    // =============================================================================
    // CONSTRUTORES E DESTRUTOR
    // =============================================================================

    GrfFile::~GrfFile()
    {
        Close();
    }

    GrfFile::GrfFile(GrfFile &&other) noexcept
        : m_filePath(std::move(other.m_filePath)), m_isOpen(other.m_isOpen), m_isModified(other.m_isModified), m_header(other.m_header), m_entries(std::move(other.m_entries)), m_hasCustomKey(other.m_hasCustomKey), m_customKey(other.m_customKey), m_lastError(other.m_lastError), m_lastErrorMessage(std::move(other.m_lastErrorMessage))
    {
        if (other.m_fileStream.is_open())
        {
            // Não podemos mover fstream, então reabrimos
            m_fileStream.open(m_filePath, std::ios::in | std::ios::out | std::ios::binary);
            other.m_fileStream.close();
        }
        other.m_isOpen = false;
    }

    GrfFile &GrfFile::operator=(GrfFile &&other) noexcept
    {
        if (this != &other)
        {
            Close();

            m_filePath = std::move(other.m_filePath);
            m_isOpen = other.m_isOpen;
            m_isModified = other.m_isModified;
            m_header = other.m_header;
            m_entries = std::move(other.m_entries);
            m_hasCustomKey = other.m_hasCustomKey;
            m_customKey = other.m_customKey;
            m_lastError = other.m_lastError;
            m_lastErrorMessage = std::move(other.m_lastErrorMessage);

            if (other.m_fileStream.is_open())
            {
                m_fileStream.open(m_filePath, std::ios::in | std::ios::out | std::ios::binary);
                other.m_fileStream.close();
            }
            other.m_isOpen = false;
        }
        return *this;
    }

    // =============================================================================
    // OPERAÇÕES DE ARQUIVO
    // =============================================================================

    GrfError GrfFile::Open(const std::string &filepath, GrfProgressCallback progressCb)
    {
        printf("[GrfFile::Open] Iniciando abertura de: %s\n", filepath.c_str());
        fflush(stdout);

        // TESTE: Removido lock temporariamente para debug
        // // TESTE: std::lock_guard<std::mutex> lock(m_mutex);

        printf("[GrfFile::Open] Prosseguindo sem lock\n");
        fflush(stdout);

        // Fecha se já estiver aberto
        if (m_isOpen)
        {
            Close();
        }

        m_filePath = filepath;

        // Verifica se arquivo existe
        printf("[GrfFile::Open] Verificando se arquivo existe...\n");
        fflush(stdout);

        if (!std::filesystem::exists(filepath))
        {
            SetError(GrfError::FILE_NOT_FOUND, "File not found: " + filepath);
            return m_lastError;
        }

        printf("[GrfFile::Open] Arquivo existe, abrindo stream...\n");
        fflush(stdout);

        // Abre arquivo
        m_fileStream.open(filepath, std::ios::in | std::ios::out | std::ios::binary);
        if (!m_fileStream.is_open())
        {
            printf("[GrfFile::Open] Falha ao abrir read/write, tentando read-only...\n");
            fflush(stdout);
            // Tenta abrir só para leitura (arquivo pode estar bloqueado para escrita)
            m_fileStream.open(filepath, std::ios::in | std::ios::binary);
            if (!m_fileStream.is_open())
            {
                SetError(GrfError::OPEN_FAILED, "Failed to open file: " + filepath);
                return m_lastError;
            }
        }

        printf("[GrfFile::Open] Stream aberto, lendo header...\n");
        fflush(stdout);

        // Lê header
        GrfError err = ReadHeader();
        if (err != GrfError::OK)
        {
            printf("[GrfFile::Open] Erro ao ler header: %d\n", static_cast<int>(err));
            fflush(stdout);
            m_fileStream.close();
            return err;
        }

        printf("[GrfFile::Open] Header OK, lendo tabela de arquivos...\n");
        fflush(stdout);

        // Lê tabela de arquivos
        err = ReadFileTable(progressCb);
        if (err != GrfError::OK)
        {
            printf("[GrfFile::Open] Erro ao ler tabela: %d\n", static_cast<int>(err));
            fflush(stdout);
            m_fileStream.close();
            return err;
        }

        printf("[GrfFile::Open] Tabela OK, GRF aberto com sucesso!\n");
        fflush(stdout);

        m_isOpen = true;
        m_isModified = false;
        SetError(GrfError::OK);

        return GrfError::OK;
    }

    GrfError GrfFile::Create(const std::string &filepath, uint32_t version)
    {
        // TESTE: std::lock_guard<std::mutex> lock(m_mutex);

        if (m_isOpen)
        {
            Close();
        }

        m_filePath = filepath;

        // Cria diretórios se necessário
        auto parent = std::filesystem::path(filepath).parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent))
        {
            std::filesystem::create_directories(parent);
        }

        // Cria arquivo
        m_fileStream.open(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!m_fileStream.is_open())
        {
            SetError(GrfError::OPEN_FAILED, "Failed to create file: " + filepath);
            return m_lastError;
        }

        // Inicializa header
        std::memset(&m_header, 0, sizeof(m_header));
        std::memcpy(m_header.magic, GrfConstants::MAGIC, GrfConstants::MAGIC_SIZE);
        m_header.version = version;
        m_header.seed = 0;
        m_header.rawFileCount = 7; // 0 + 7
        m_header.realFileCount = 0;
        m_header.fileTableOffset = GrfConstants::HEADER_SIZE;

        // Limpa entradas
        m_entries.clear();

        // Escreve header inicial
        WriteHeader();

        m_isOpen = true;
        m_isModified = true;
        SetError(GrfError::OK);

        return GrfError::OK;
    }

    void GrfFile::Close()
    {
        // TESTE: std::lock_guard<std::mutex> lock(m_mutex);

        if (m_fileStream.is_open())
        {
            m_fileStream.close();
        }

        m_entries.clear();
        m_isOpen = false;
        m_isModified = false;
        m_filePath.clear();
    }

    GrfError GrfFile::Save(GrfProgressCallback progressCb)
    {
        // Tenta QuickMerge primeiro (rápido, apenas adiciona ao final)
        GrfError result = QuickSave(progressCb);

        if (result != GrfError::OK)
        {
            // Se QuickMerge falhou, tenta FullRepack (cria nova GRF)
            printf("[GrfFile::Save] QuickMerge failed, attempting FullRepack...\n");
            fflush(stdout);
            result = FullRepack(progressCb);
        }

        return result;
    }

    GrfError GrfFile::SaveAs(const std::string &filepath, GrfProgressCallback progressCb)
    {
        // SaveAs sempre faz FullRepack para o novo arquivo
        return FullRepack(progressCb, filepath);
    }

    GrfError GrfFile::QuickSave(GrfProgressCallback progressCb)
    {
        // TESTE: std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_isOpen)
        {
            SetError(GrfError::INVALID_OPERATION, "GRF is not open");
            return m_lastError;
        }

        // Escreve dados dos arquivos (QuickMerge - apenas novos/modificados)
        GrfError err = WriteFileData(progressCb);
        if (err != GrfError::OK)
        {
            return err;
        }

        // Atualiza header
        m_header.realFileCount = 0;
        for (const auto &pair : m_entries)
        {
            if (!pair.second.isDeleted)
            {
                m_header.realFileCount++;
            }
        }
        m_header.rawFileCount = m_header.realFileCount + m_header.seed + 7;

        // Escreve tabela de arquivos
        err = WriteFileTable();
        if (err != GrfError::OK)
        {
            return err;
        }

        // Escreve header
        err = WriteHeader();
        if (err != GrfError::OK)
        {
            return err;
        }

        m_isModified = false;
        SetError(GrfError::OK);

        return GrfError::OK;
    }

    GrfError GrfFile::FullRepack(GrfProgressCallback progressCb, const std::string &outputPath)
    {
        /**
         * FullRepack - Recria a GRF do zero
         *
         * Usado como fallback quando QuickMerge falha ou para SaveAs.
         * Este método:
         * 1. Cria um arquivo temporário
         * 2. Copia TODOS os arquivos (existentes + novos) para o novo arquivo
         * 3. Substitui o arquivo original
         */

        if (!m_isOpen)
        {
            SetError(GrfError::INVALID_OPERATION, "GRF is not open");
            return m_lastError;
        }

        std::string targetPath = outputPath.empty() ? m_filePath : outputPath;
        std::string tempPath = targetPath + ".tmp";

        printf("[GrfFile::FullRepack] Creating temp file: %s\n", tempPath.c_str());
        fflush(stdout);

        // Cria arquivo temporário
        std::fstream tempFile(tempPath, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!tempFile.is_open())
        {
            SetError(GrfError::OPEN_FAILED, "Failed to create temp file: " + tempPath);
            return m_lastError;
        }

        // Conta arquivos válidos
        int totalFiles = 0;
        for (const auto &pair : m_entries)
        {
            if (!pair.second.isDeleted)
            {
                totalFiles++;
            }
        }

        int current = 0;
        uint64_t writeOffset = 0;

        // Escreve header placeholder (será reescrito no final)
        char headerPlaceholder[GrfConstants::HEADER_SIZE] = {0};
        tempFile.write(headerPlaceholder, GrfConstants::HEADER_SIZE);

        // Copia TODOS os arquivos para o novo arquivo
        for (auto &pair : m_entries)
        {
            GrfEntry &entry = pair.second;

            if (entry.isDeleted)
            {
                continue;
            }

            if (progressCb)
            {
                if (!progressCb(current, totalFiles, "Repacking: " + entry.filename))
                {
                    tempFile.close();
                    std::filesystem::remove(tempPath);
                    return GrfError::OK; // Cancelado
                }
            }

            std::vector<uint8_t> dataToWrite;

            if (entry.isNew || entry.isModified || !entry.cachedData.empty())
            {
                // Arquivo novo/modificado - usa dados do cache
                dataToWrite = entry.cachedData;
            }
            else
            {
                // Arquivo existente - lê do GRF original
                m_fileStream.seekg(static_cast<std::streamoff>(GrfConstants::HEADER_SIZE + entry.offset));
                dataToWrite.resize(entry.sizeCompressedAligned);
                m_fileStream.read(reinterpret_cast<char *>(dataToWrite.data()), entry.sizeCompressedAligned);

                if (!m_fileStream)
                {
                    printf("[GrfFile::FullRepack] Failed to read existing file: %s\n", entry.filename.c_str());
                    fflush(stdout);
                    tempFile.close();
                    std::filesystem::remove(tempPath);
                    SetError(GrfError::READ_FAILED, "Failed to read file: " + entry.filename);
                    return m_lastError;
                }
            }

            // Padding para alinhamento (múltiplo de 8 bytes)
            size_t paddedSize = (dataToWrite.size() + 7) & ~7;
            dataToWrite.resize(paddedSize, 0);

            // Escreve no arquivo temporário
            tempFile.write(reinterpret_cast<char *>(dataToWrite.data()), dataToWrite.size());

            if (!tempFile)
            {
                tempFile.close();
                std::filesystem::remove(tempPath);
                SetError(GrfError::WRITE_FAILED, "Failed to write file: " + entry.filename);
                return m_lastError;
            }

            // Atualiza offset da entrada
            entry.offset = writeOffset;
            entry.sizeCompressedAligned = static_cast<uint32_t>(paddedSize);
            entry.isModified = false;
            entry.isNew = false;

            writeOffset += paddedSize;
            current++;
        }

        // Atualiza header
        m_header.fileTableOffset = writeOffset;
        m_header.realFileCount = totalFiles;
        m_header.rawFileCount = m_header.realFileCount + m_header.seed + 7;

        // Escreve tabela de arquivos no arquivo temporário
        std::vector<uint8_t> tableData;
        for (const auto &pair : m_entries)
        {
            const GrfEntry &entry = pair.second;
            if (entry.isDeleted)
                continue;

            const std::string &filename = entry.filename;
            tableData.insert(tableData.end(), filename.begin(), filename.end());
            tableData.push_back(0);

            auto pushUint32 = [&tableData](uint32_t value)
            {
                tableData.push_back(value & 0xFF);
                tableData.push_back((value >> 8) & 0xFF);
                tableData.push_back((value >> 16) & 0xFF);
                tableData.push_back((value >> 24) & 0xFF);
            };

            pushUint32(entry.sizeCompressed);
            pushUint32(entry.sizeCompressedAligned);
            pushUint32(entry.sizeDecompressed);
            tableData.push_back(entry.flags);
            pushUint32(static_cast<uint32_t>(entry.offset));
        }

        std::vector<uint8_t> compressedTable = Compression::Compress(tableData);

        uint32_t tableSizeCompressed = static_cast<uint32_t>(compressedTable.size());
        uint32_t tableSize = static_cast<uint32_t>(tableData.size());

        tempFile.write(reinterpret_cast<char *>(&tableSizeCompressed), 4);
        tempFile.write(reinterpret_cast<char *>(&tableSize), 4);
        tempFile.write(reinterpret_cast<char *>(compressedTable.data()), compressedTable.size());

        // Escreve header no início
        tempFile.seekp(0);
        tempFile.write(m_header.magic, GrfConstants::MAGIC_SIZE);
        tempFile.write(m_header.key, GrfConstants::KEY_SIZE);

        uint32_t tableOffset32 = static_cast<uint32_t>(m_header.fileTableOffset);
        tempFile.write(reinterpret_cast<char *>(&tableOffset32), 4);
        tempFile.write(reinterpret_cast<char *>(&m_header.seed), 4);
        tempFile.write(reinterpret_cast<char *>(&m_header.rawFileCount), 4);
        tempFile.write(reinterpret_cast<char *>(&m_header.version), 4);

        tempFile.close();

        // Fecha o GRF original
        m_fileStream.close();

        // Substitui o arquivo original pelo temporário
        try
        {
            std::filesystem::remove(targetPath);
            std::filesystem::rename(tempPath, targetPath);
        }
        catch (const std::exception &e)
        {
            SetError(GrfError::WRITE_FAILED, "Failed to replace GRF: " + std::string(e.what()));
            return m_lastError;
        }

        // Reabre o GRF
        m_filePath = targetPath;
        m_fileStream.open(targetPath, std::ios::in | std::ios::out | std::ios::binary);
        if (!m_fileStream.is_open())
        {
            SetError(GrfError::OPEN_FAILED, "Failed to reopen GRF after repack");
            return m_lastError;
        }

        m_isModified = false;
        SetError(GrfError::OK);

        printf("[GrfFile::FullRepack] Success!\n");
        fflush(stdout);

        return GrfError::OK;
    }

    // =============================================================================
    // MANIPULAÇÃO DE ENTRADAS
    // =============================================================================

    std::string GrfFile::NormalizeFilename(const std::string &filename)
    {
        std::string normalized = filename;

        // Converte para lowercase usando tolower padrão
        // Isso funciona corretamente para EUC-KR porque tolower só afeta A-Z (0x41-0x5A)
        // e bytes acima de 0x7F não são alterados pelo tolower padrão do C
        std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                       [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });

        // Converte '/' para '\\'
        std::replace(normalized.begin(), normalized.end(), '/', '\\');

        // Remove '\\' inicial se existir
        if (!normalized.empty() && normalized[0] == '\\')
        {
            normalized = normalized.substr(1);
        }

        return normalized;
    }

    GrfEntry *GrfFile::FindEntry(const std::string &filename)
    {
        std::string normalized = NormalizeFilename(filename);
        auto it = m_entries.find(normalized);
        if (it != m_entries.end() && !it->second.isDeleted)
        {
            return &it->second;
        }
        return nullptr;
    }

    const GrfEntry *GrfFile::FindEntry(const std::string &filename) const
    {
        std::string normalized = NormalizeFilename(filename);
        auto it = m_entries.find(normalized);
        if (it != m_entries.end() && !it->second.isDeleted)
        {
            return &it->second;
        }
        return nullptr;
    }

    bool GrfFile::FileExists(const std::string &filename) const
    {
        return FindEntry(filename) != nullptr;
    }

    std::vector<std::string> GrfFile::GetFileList() const
    {
        std::vector<std::string> files;
        files.reserve(m_entries.size());

        for (const auto &pair : m_entries)
        {
            if (!pair.second.isDeleted && pair.second.IsFile())
            {
                files.push_back(pair.second.filename);
            }
        }

        return files;
    }

    bool GrfFile::MatchesFilter(const std::string &filename, const std::string &filter)
    {
        // Converte filtro de wildcard para regex
        std::string pattern = filter;

        // Escape caracteres especiais de regex
        pattern = std::regex_replace(pattern, std::regex("\\."), "\\.");
        pattern = std::regex_replace(pattern, std::regex("\\*"), ".*");
        pattern = std::regex_replace(pattern, std::regex("\\?"), ".");

        // Converte separadores
        std::replace(pattern.begin(), pattern.end(), '/', '\\');

        try
        {
            std::regex re(pattern, std::regex::icase);
            return std::regex_match(filename, re);
        }
        catch (...)
        {
            return false;
        }
    }

    std::vector<std::string> GrfFile::GetFileList(const std::string &filter) const
    {
        std::vector<std::string> files;

        for (const auto &pair : m_entries)
        {
            if (!pair.second.isDeleted && pair.second.IsFile())
            {
                if (MatchesFilter(pair.second.filename, filter))
                {
                    files.push_back(pair.second.filename);
                }
            }
        }

        return files;
    }

    // =============================================================================
    // EXTRAÇÃO DE ARQUIVOS
    // =============================================================================

    std::vector<uint8_t> GrfFile::ExtractFile(const std::string &filename)
    {
        // TESTE: std::lock_guard<std::mutex> lock(m_mutex);

        GrfEntry *entry = FindEntry(filename);
        if (!entry)
        {
            SetError(GrfError::ENTRY_NOT_FOUND, "File not found: " + filename);
            return {};
        }

        // Se tem dados em cache (arquivo modificado/novo), retorna
        if (!entry->cachedData.empty())
        {
            return entry->cachedData;
        }

        // Lê dados do arquivo
        if (!entry->IsFile() || entry->sizeCompressed == 0)
        {
            return {};
        }

        // Posiciona no offset
        m_fileStream.seekg(static_cast<std::streamoff>(GrfConstants::HEADER_SIZE + entry->offset));
        if (!m_fileStream)
        {
            SetError(GrfError::READ_FAILED, "Failed to seek to file data");
            return {};
        }

        // Lê dados comprimidos/criptografados
        std::vector<uint8_t> compressedData(entry->sizeCompressedAligned);
        m_fileStream.read(reinterpret_cast<char *>(compressedData.data()), entry->sizeCompressedAligned);
        if (!m_fileStream)
        {
            SetError(GrfError::READ_FAILED, "Failed to read file data");
            return {};
        }

        // Descriptografa se necessário
        if (entry->IsEncrypted())
        {
            DecryptEntry(*entry, compressedData);
        }

        // Descomprime
        std::vector<uint8_t> decompressedData(entry->sizeDecompressed);

        if (entry->sizeCompressed != entry->sizeDecompressed)
        {
            // Dados comprimidos - descomprime
            size_t outSize = entry->sizeDecompressed;
            if (!Compression::Decompress(compressedData.data(), entry->sizeCompressed,
                                         decompressedData.data(), outSize))
            {
                SetError(GrfError::DECOMPRESS_FAILED, "Failed to decompress file");
                return {};
            }
            decompressedData.resize(outSize);
        }
        else
        {
            // Dados não comprimidos
            decompressedData = std::move(compressedData);
            decompressedData.resize(entry->sizeDecompressed);
        }

        SetError(GrfError::OK);
        return decompressedData;
    }

    GrfError GrfFile::ExtractToFile(const std::string &filename, const std::string &outputPath)
    {
        auto data = ExtractFile(filename);
        if (data.empty() && m_lastError != GrfError::OK)
        {
            return m_lastError;
        }

        // Cria diretórios
        auto parent = std::filesystem::path(outputPath).parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent))
        {
            std::filesystem::create_directories(parent);
        }

        // Escreve arquivo
        std::ofstream out(outputPath, std::ios::binary);
        if (!out)
        {
            SetError(GrfError::WRITE_FAILED, "Failed to create output file");
            return m_lastError;
        }

        out.write(reinterpret_cast<const char *>(data.data()), data.size());
        out.close();

        return GrfError::OK;
    }

    GrfError GrfFile::ExtractAll(const std::string &outputDir, GrfProgressCallback progressCb)
    {
        // TESTE: std::lock_guard<std::mutex> lock(m_mutex);

        auto files = GetFileList();
        int current = 0;
        int total = static_cast<int>(files.size());

        for (const auto &file : files)
        {
            if (progressCb)
            {
                if (!progressCb(current, total, file))
                {
                    return GrfError::OK; // Cancelado
                }
            }

            std::string outputPath = outputDir + "\\" + file;
            GrfError err = ExtractToFile(file, outputPath);
            if (err != GrfError::OK)
            {
                // Continua mesmo com erros
            }

            current++;
        }

        return GrfError::OK;
    }

    // =============================================================================
    // ADIÇÃO/MODIFICAÇÃO DE ARQUIVOS
    // =============================================================================

    GrfError GrfFile::AddFile(const std::string &filename, const std::vector<uint8_t> &data,
                              bool compress)
    {
        // TESTE: std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_isOpen)
        {
            SetError(GrfError::INVALID_OPERATION, "GRF is not open");
            return m_lastError;
        }

        std::string normalized = NormalizeFilename(filename);

        // Verifica se já existe (para diferenciar isNew de isModified)
        bool alreadyExists = (m_entries.find(normalized) != m_entries.end() &&
                              !m_entries[normalized].isDeleted);

        // Cria ou atualiza entrada
        GrfEntry &entry = m_entries[normalized];
        entry.filename = normalized;
        entry.sizeDecompressed = static_cast<uint32_t>(data.size());
        entry.flags = GrfConstants::FLAG_FILE;

        if (compress && data.size() > 128)
        {
            // Comprime dados
            entry.cachedData = Compression::Compress(data);
            entry.sizeCompressed = static_cast<uint32_t>(entry.cachedData.size());

            // Se não comprimiu bem, usa dados originais
            if (entry.cachedData.size() >= data.size())
            {
                entry.cachedData = data;
                entry.sizeCompressed = entry.sizeDecompressed;
            }
        }
        else
        {
            entry.cachedData = data;
            entry.sizeCompressed = entry.sizeDecompressed;
        }

        entry.sizeCompressedAligned = (entry.sizeCompressed + 7) & ~7; // Alinha para 8 bytes
        entry.isModified = true;
        entry.isNew = !alreadyExists; // Só é "novo" se não existia antes
        entry.isDeleted = false;
        entry.cycle = -1;

        m_isModified = true;
        SetError(GrfError::OK);

        return GrfError::OK;
    }

    GrfError GrfFile::AddFileFromDisk(const std::string &grfPath, const std::string &diskPath,
                                      bool compress)
    {
        // Lê arquivo do disco
        std::ifstream file(diskPath, std::ios::binary | std::ios::ate);
        if (!file)
        {
            SetError(GrfError::FILE_NOT_FOUND, "File not found: " + diskPath);
            return m_lastError;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(static_cast<size_t>(size));
        file.read(reinterpret_cast<char *>(data.data()), size);
        file.close();

        return AddFile(grfPath, data, compress);
    }

    GrfError GrfFile::AddDirectory(const std::string &basePath, const std::string &diskDir,
                                   GrfProgressCallback progressCb)
    {
        if (!std::filesystem::exists(diskDir))
        {
            SetError(GrfError::FILE_NOT_FOUND, "Directory not found: " + diskDir);
            return m_lastError;
        }

        std::vector<std::filesystem::path> files;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(diskDir))
        {
            if (entry.is_regular_file())
            {
                files.push_back(entry.path());
            }
        }

        int current = 0;
        int total = static_cast<int>(files.size());

        for (const auto &file : files)
        {
            if (progressCb)
            {
                if (!progressCb(current, total, file.string()))
                {
                    return GrfError::OK; // Cancelado
                }
            }

            // Calcula caminho relativo
            std::string relativePath = std::filesystem::relative(file, diskDir).string();
            std::string grfPath = basePath.empty() ? relativePath : basePath + "\\" + relativePath;

            GrfError err = AddFileFromDisk(grfPath, file.string());
            if (err != GrfError::OK)
            {
                // Continua mesmo com erros
            }

            current++;
        }

        return GrfError::OK;
    }

    GrfError GrfFile::RemoveFile(const std::string &filename)
    {
        // TESTE: std::lock_guard<std::mutex> lock(m_mutex);

        GrfEntry *entry = FindEntry(filename);
        if (!entry)
        {
            SetError(GrfError::ENTRY_NOT_FOUND, "File not found: " + filename);
            return m_lastError;
        }

        entry->isDeleted = true;
        m_isModified = true;
        SetError(GrfError::OK);

        return GrfError::OK;
    }

    GrfError GrfFile::RenameFile(const std::string &oldName, const std::string &newName)
    {
        // TESTE: std::lock_guard<std::mutex> lock(m_mutex);

        std::string oldNorm = NormalizeFilename(oldName);
        std::string newNorm = NormalizeFilename(newName);

        auto it = m_entries.find(oldNorm);
        if (it == m_entries.end() || it->second.isDeleted)
        {
            SetError(GrfError::ENTRY_NOT_FOUND, "File not found: " + oldName);
            return m_lastError;
        }

        // Verifica se novo nome já existe
        if (m_entries.find(newNorm) != m_entries.end())
        {
            SetError(GrfError::INVALID_OPERATION, "File already exists: " + newName);
            return m_lastError;
        }

        // Move entrada
        GrfEntry entry = std::move(it->second);
        m_entries.erase(it);

        entry.filename = newNorm;
        entry.isModified = true;
        m_entries[newNorm] = std::move(entry);

        m_isModified = true;
        SetError(GrfError::OK);

        return GrfError::OK;
    }

    // =============================================================================
    // CRIPTOGRAFIA
    // =============================================================================

    void GrfFile::SetEncryptionKey(const std::array<uint8_t, 256> &key)
    {
        // TESTE: std::lock_guard<std::mutex> lock(m_mutex);
        m_customKey = key;
        m_hasCustomKey = true;
    }

    void GrfFile::ClearEncryptionKey()
    {
        // TESTE: std::lock_guard<std::mutex> lock(m_mutex);
        m_hasCustomKey = false;
        std::memset(m_customKey.data(), 0, m_customKey.size());
    }

    void GrfFile::DecryptEntry(GrfEntry &entry, std::vector<uint8_t> &data)
    {
        // Aplica descriptografia customizada primeiro (se houver)
        if (m_hasCustomKey)
        {
            for (size_t i = 0; i < data.size(); ++i)
            {
                data[i] ^= m_customKey[i % 256];
            }
        }

        // Aplica DES se necessário (versões antigas)
        if (entry.IsEncrypted() && (m_header.version == GrfConstants::VERSION_102 ||
                                    m_header.version == GrfConstants::VERSION_103))
        {
            int type = entry.flags & (GrfConstants::FLAG_ENCRYPT_MIXED | GrfConstants::FLAG_ENCRYPT_HEADER);
            DESCrypto::DecryptFileData(data, type, entry.cycle);
        }
    }

    void GrfFile::EncryptEntry(GrfEntry &entry, std::vector<uint8_t> &data)
    {
        // Aplica DES se necessário (versões antigas)
        if (entry.IsEncrypted() && (m_header.version == GrfConstants::VERSION_102 ||
                                    m_header.version == GrfConstants::VERSION_103))
        {
            int type = entry.flags & (GrfConstants::FLAG_ENCRYPT_MIXED | GrfConstants::FLAG_ENCRYPT_HEADER);
            DESCrypto::EncryptFileData(data, type, entry.cycle);
        }

        // Aplica criptografia customizada (se houver)
        if (m_hasCustomKey)
        {
            for (size_t i = 0; i < data.size(); ++i)
            {
                data[i] ^= m_customKey[i % 256];
            }
        }
    }

    // =============================================================================
    // LEITURA DE GRF
    // =============================================================================

    GrfError GrfFile::ReadHeader()
    {
        m_fileStream.seekg(0);

        char magic[GrfConstants::MAGIC_SIZE];
        m_fileStream.read(magic, GrfConstants::MAGIC_SIZE);

        if (std::memcmp(magic, GrfConstants::MAGIC, GrfConstants::MAGIC_SIZE - 1) != 0)
        {
            SetError(GrfError::INVALID_MAGIC, "Invalid GRF magic header");
            return m_lastError;
        }

        std::memcpy(m_header.magic, magic, GrfConstants::MAGIC_SIZE);

        // Lê encryption key
        m_fileStream.read(m_header.key, GrfConstants::KEY_SIZE);

        // Lê offset da tabela de arquivos (4 bytes para v1/v2, 8 para v3)
        uint32_t tableOffset32;
        m_fileStream.read(reinterpret_cast<char *>(&tableOffset32), 4);
        m_header.fileTableOffset = tableOffset32;

        // Lê seed e file count
        m_fileStream.read(reinterpret_cast<char *>(&m_header.seed), 4);
        m_fileStream.read(reinterpret_cast<char *>(&m_header.rawFileCount), 4);

        // Lê versão
        m_fileStream.read(reinterpret_cast<char *>(&m_header.version), 4);

        // Calcula número real de arquivos
        m_header.realFileCount = m_header.rawFileCount - m_header.seed - 7;

        // Verifica versão suportada
        if (m_header.version != GrfConstants::VERSION_102 &&
            m_header.version != GrfConstants::VERSION_103 &&
            m_header.version != GrfConstants::VERSION_200 &&
            m_header.version != GrfConstants::VERSION_300)
        {
            SetError(GrfError::UNSUPPORTED_VERSION,
                     "Unsupported GRF version: 0x" + std::to_string(m_header.version));
            return m_lastError;
        }

        return GrfError::OK;
    }

    GrfError GrfFile::ReadFileTable(GrfProgressCallback progressCb)
    {
        // Posiciona no offset da tabela
        m_fileStream.seekg(static_cast<std::streamoff>(GrfConstants::HEADER_SIZE + m_header.fileTableOffset));
        if (!m_fileStream)
        {
            SetError(GrfError::CORRUPT_FILE_TABLE, "Failed to seek to file table");
            return m_lastError;
        }

        // Lê tamanhos da tabela
        uint32_t tableSizeCompressed, tableSize;
        m_fileStream.read(reinterpret_cast<char *>(&tableSizeCompressed), 4);
        m_fileStream.read(reinterpret_cast<char *>(&tableSize), 4);

        if (tableSizeCompressed == 0 || tableSize == 0)
        {
            // GRF vazio
            return GrfError::OK;
        }

        // Lê dados comprimidos da tabela
        std::vector<uint8_t> compressedTable(tableSizeCompressed);
        m_fileStream.read(reinterpret_cast<char *>(compressedTable.data()), tableSizeCompressed);
        if (!m_fileStream)
        {
            SetError(GrfError::READ_FAILED, "Failed to read file table");
            return m_lastError;
        }

        // Descomprime tabela
        std::vector<uint8_t> tableData(tableSize);
        size_t outSize = tableSize;
        if (!Compression::Decompress(compressedTable.data(), tableSizeCompressed,
                                     tableData.data(), outSize))
        {
            SetError(GrfError::DECOMPRESS_FAILED, "Failed to decompress file table");
            return m_lastError;
        }

        // Parseia entradas baseado na versão
        if (m_header.version == GrfConstants::VERSION_102 ||
            m_header.version == GrfConstants::VERSION_103)
        {
            return ReadFileTableV1(tableData, progressCb);
        }
        else
        {
            return ReadFileTableV2(tableData, progressCb);
        }
    }

    GrfError GrfFile::ReadFileTableV1(const std::vector<uint8_t> &tableData, GrfProgressCallback progressCb)
    {
        const uint8_t *ptr = tableData.data();
        const uint8_t *end = ptr + tableData.size();

        int current = 0;
        int total = m_header.realFileCount;

        while (ptr < end && current < total)
        {
            if (progressCb)
            {
                if (!progressCb(current, total, "Loading file table..."))
                {
                    return GrfError::OK; // Cancelado
                }
            }

            GrfEntry entry;

            // Nome do arquivo (null-terminated, possivelmente criptografado)
            const uint8_t *nameStart = ptr;
            while (ptr < end && *ptr != 0)
                ptr++;

            size_t nameLen = ptr - nameStart;
            if (ptr < end)
                ptr++; // Pula null terminator

            // Descriptografa nome se necessário
            entry.filename = DESCrypto::DecodeFileName(nameStart, nameLen);
            entry.filename = NormalizeFilename(entry.filename);

            if (ptr + 17 > end)
                break;

            // Lê atributos
            entry.sizeCompressed = *reinterpret_cast<const uint32_t *>(ptr);
            ptr += 4;
            entry.sizeCompressedAligned = *reinterpret_cast<const uint32_t *>(ptr);
            ptr += 4;
            entry.sizeDecompressed = *reinterpret_cast<const uint32_t *>(ptr);
            ptr += 4;
            entry.flags = *ptr++;
            ptr += 3; // 3 bytes padding
            entry.offset = *reinterpret_cast<const uint32_t *>(ptr);
            ptr += 4;

            // Determina ciclo de criptografia baseado no tamanho
            if (entry.IsEncrypted())
            {
                entry.cycle = 1;
                if (entry.sizeCompressed >= 3)
                {
                    entry.cycle = entry.sizeCompressed / 3;
                    if (entry.cycle < 1)
                        entry.cycle = 1;
                }
            }

            // Adiciona à lista
            if (!entry.filename.empty())
            {
                m_entries[entry.filename] = std::move(entry);
            }

            current++;
        }

        return GrfError::OK;
    }

    GrfError GrfFile::ReadFileTableV2(const std::vector<uint8_t> &tableData, GrfProgressCallback progressCb)
    {
        const uint8_t *ptr = tableData.data();
        const uint8_t *end = ptr + tableData.size();

        int current = 0;
        int total = m_header.realFileCount;

        while (ptr < end && current < total)
        {
            if (progressCb && (current % 1000 == 0))
            {
                if (!progressCb(current, total, "Loading file table..."))
                {
                    return GrfError::OK; // Cancelado
                }
            }

            GrfEntry entry;

            // Nome do arquivo (null-terminated)
            const char *nameStart = reinterpret_cast<const char *>(ptr);
            while (ptr < end && *ptr != 0)
                ptr++;

            size_t nameLen = ptr - reinterpret_cast<const uint8_t *>(nameStart);
            if (ptr < end)
                ptr++; // Pula null terminator

            entry.filename = std::string(nameStart, nameLen);
            entry.filename = NormalizeFilename(entry.filename);

            if (ptr + 17 > end)
                break;

            // Lê atributos
            entry.sizeCompressed = *reinterpret_cast<const uint32_t *>(ptr);
            ptr += 4;
            entry.sizeCompressedAligned = *reinterpret_cast<const uint32_t *>(ptr);
            ptr += 4;
            entry.sizeDecompressed = *reinterpret_cast<const uint32_t *>(ptr);
            ptr += 4;
            entry.flags = *ptr++;
            entry.offset = *reinterpret_cast<const uint32_t *>(ptr);
            ptr += 4;

            entry.cycle = -1; // Sem criptografia DES na v2

            // Adiciona à lista
            if (!entry.filename.empty())
            {
                m_entries[entry.filename] = std::move(entry);
            }

            current++;
        }

        return GrfError::OK;
    }

    // =============================================================================
    // ESCRITA DE GRF
    // =============================================================================

    GrfError GrfFile::WriteHeader()
    {
        m_fileStream.seekp(0);

        m_fileStream.write(m_header.magic, GrfConstants::MAGIC_SIZE);
        m_fileStream.write(m_header.key, GrfConstants::KEY_SIZE);

        uint32_t tableOffset32 = static_cast<uint32_t>(m_header.fileTableOffset);
        m_fileStream.write(reinterpret_cast<char *>(&tableOffset32), 4);
        m_fileStream.write(reinterpret_cast<char *>(&m_header.seed), 4);
        m_fileStream.write(reinterpret_cast<char *>(&m_header.rawFileCount), 4);
        m_fileStream.write(reinterpret_cast<char *>(&m_header.version), 4);

        if (!m_fileStream)
        {
            SetError(GrfError::WRITE_FAILED, "Failed to write header");
            return m_lastError;
        }

        return GrfError::OK;
    }

    GrfError GrfFile::WriteFileData(GrfProgressCallback progressCb)
    {
        /**
         * QuickMerge Implementation (baseado no GRF Editor do Tokei)
         *
         * A estratégia correta para não corromper o GRF:
         * 1. NÃO modificar dados de arquivos existentes - seus offsets permanecem inalterados
         * 2. Encontrar onde termina o último arquivo existente
         * 3. Adicionar novos arquivos APENAS ao final
         * 4. Reescrever apenas a tabela de arquivos e header
         *
         * Isso evita corrupção porque:
         * - Arquivos existentes mantêm seus offsets originais na tabela
         * - Novos arquivos são simplesmente anexados ao final
         * - A tabela de arquivos é sempre reescrita com todos os offsets corretos
         */

        // Passo 1: Encontrar onde termina o último arquivo existente (não deletado, não novo)
        uint64_t endOffset = 0;

        for (const auto &pair : m_entries)
        {
            const GrfEntry &entry = pair.second;

            // Ignorar arquivos deletados ou novos
            if (entry.isDeleted || entry.isNew)
            {
                continue;
            }

            // Calcular onde termina este arquivo
            uint64_t entryEnd = entry.offset + entry.sizeCompressedAligned;
            if (entryEnd > endOffset)
            {
                endOffset = entryEnd;
            }
        }

        // Se o GRF está vazio (novo), começar após o header
        if (endOffset == 0)
        {
            endOffset = 0; // Offset é relativo ao header, então 0 = logo após header
        }

        // Passo 2: Contar quantos arquivos novos/modificados temos
        int newFilesCount = 0;
        for (const auto &pair : m_entries)
        {
            if (!pair.second.isDeleted && (pair.second.isNew || pair.second.isModified))
            {
                newFilesCount++;
            }
        }

        int current = 0;
        int total = newFilesCount;

        // Passo 3: Escrever APENAS arquivos novos/modificados ao final
        uint64_t writeOffset = endOffset;

        for (auto &pair : m_entries)
        {
            GrfEntry &entry = pair.second;

            if (entry.isDeleted)
            {
                continue;
            }

            // Apenas processar arquivos novos ou modificados
            if (!entry.isNew && !entry.isModified)
            {
                // Arquivo existente - NÃO TOCAR, manter offset original
                continue;
            }

            if (progressCb)
            {
                if (!progressCb(current, total, "Writing: " + entry.filename))
                {
                    return GrfError::OK; // Cancelado
                }
            }

            // Preparar dados para escrita
            std::vector<uint8_t> dataToWrite = entry.cachedData;

            // Aplica criptografia se necessário
            if (entry.IsEncrypted())
            {
                EncryptEntry(entry, dataToWrite);
            }

            // Padding para alinhamento (múltiplo de 8 bytes)
            size_t paddedSize = (dataToWrite.size() + 7) & ~7;
            dataToWrite.resize(paddedSize, 0);

            // Posicionar e escrever ao final do GRF
            m_fileStream.seekp(static_cast<std::streamoff>(GrfConstants::HEADER_SIZE + writeOffset));
            m_fileStream.write(reinterpret_cast<char *>(dataToWrite.data()), dataToWrite.size());

            if (!m_fileStream)
            {
                SetError(GrfError::WRITE_FAILED, "Failed to write file data: " + entry.filename);
                return m_lastError;
            }

            // Atualizar metadados da entrada
            entry.offset = writeOffset;
            entry.sizeCompressedAligned = static_cast<uint32_t>(paddedSize);
            entry.isModified = false;
            entry.isNew = false;

            // Avançar offset de escrita
            writeOffset += paddedSize;
            current++;
        }

        // Passo 4: Atualizar offset da tabela de arquivos
        // A tabela será escrita logo após o último arquivo
        m_header.fileTableOffset = writeOffset;

        return GrfError::OK;
    }

    GrfError GrfFile::WriteFileTable()
    {
        // Constrói tabela de arquivos
        std::vector<uint8_t> tableData;

        for (const auto &pair : m_entries)
        {
            const GrfEntry &entry = pair.second;

            if (entry.isDeleted)
            {
                continue;
            }

            // Nome do arquivo + null terminator
            const std::string &filename = entry.filename;
            tableData.insert(tableData.end(), filename.begin(), filename.end());
            tableData.push_back(0);

            // Atributos
            auto pushUint32 = [&tableData](uint32_t value)
            {
                tableData.push_back(value & 0xFF);
                tableData.push_back((value >> 8) & 0xFF);
                tableData.push_back((value >> 16) & 0xFF);
                tableData.push_back((value >> 24) & 0xFF);
            };

            pushUint32(entry.sizeCompressed);
            pushUint32(entry.sizeCompressedAligned);
            pushUint32(entry.sizeDecompressed);
            tableData.push_back(entry.flags);
            pushUint32(static_cast<uint32_t>(entry.offset));
        }

        // Comprime tabela
        std::vector<uint8_t> compressedTable = Compression::Compress(tableData);

        // Posiciona e escreve
        m_fileStream.seekp(static_cast<std::streamoff>(GrfConstants::HEADER_SIZE + m_header.fileTableOffset));

        uint32_t tableSizeCompressed = static_cast<uint32_t>(compressedTable.size());
        uint32_t tableSize = static_cast<uint32_t>(tableData.size());

        m_fileStream.write(reinterpret_cast<char *>(&tableSizeCompressed), 4);
        m_fileStream.write(reinterpret_cast<char *>(&tableSize), 4);
        m_fileStream.write(reinterpret_cast<char *>(compressedTable.data()), compressedTable.size());

        if (!m_fileStream)
        {
            SetError(GrfError::WRITE_FAILED, "Failed to write file table");
            return m_lastError;
        }

        return GrfError::OK;
    }

    // =============================================================================
    // DIAGNÓSTICO
    // =============================================================================

    std::vector<std::string> GrfFile::VerifyIntegrity(GrfProgressCallback progressCb)
    {
        std::vector<std::string> problems;

        int current = 0;
        int total = static_cast<int>(m_entries.size());

        for (const auto &pair : m_entries)
        {
            const GrfEntry &entry = pair.second;

            if (entry.isDeleted)
            {
                continue;
            }

            if (progressCb && (current % 100 == 0))
            {
                if (!progressCb(current, total, "Verifying: " + entry.filename))
                {
                    break; // Cancelado
                }
            }

            // Tenta extrair o arquivo
            auto data = ExtractFile(entry.filename);
            if (data.empty() && m_lastError != GrfError::OK)
            {
                problems.push_back(entry.filename + ": " + GrfErrorToString(m_lastError));
            }
            else if (data.size() != entry.sizeDecompressed)
            {
                problems.push_back(entry.filename + ": Size mismatch");
            }

            current++;
        }

        return problems;
    }

    std::string GrfFile::CalculateFileMD5(const std::string &filename)
    {
        auto data = ExtractFile(filename);
        if (data.empty())
        {
            return "";
        }

#ifdef _WIN32
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;

        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        {
            return "";
        }

        if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
        {
            CryptReleaseContext(hProv, 0);
            return "";
        }

        if (!CryptHashData(hHash, data.data(), static_cast<DWORD>(data.size()), 0))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return "";
        }

        BYTE hash[16];
        DWORD hashLen = 16;
        if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return "";
        }

        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);

        // Converte para hex
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (int i = 0; i < 16; ++i)
        {
            ss << std::setw(2) << static_cast<int>(hash[i]);
        }

        return ss.str();
#else
        return ""; // TODO: Implementar para outras plataformas
#endif
    }

    GrfFile::Statistics GrfFile::GetStatistics() const
    {
        Statistics stats;

        for (const auto &pair : m_entries)
        {
            const GrfEntry &entry = pair.second;

            if (entry.isDeleted)
            {
                continue;
            }

            stats.totalFiles++;
            stats.totalSize += entry.sizeDecompressed;
            stats.compressedSize += entry.sizeCompressed;

            if (entry.IsEncrypted())
            {
                stats.encryptedFiles++;
            }
        }

        return stats;
    }

    void GrfFile::SetError(GrfError error, const std::string &message)
    {
        m_lastError = error;
        m_lastErrorMessage = message.empty() ? GrfErrorToString(error) : message;
    }

} // namespace Panic
