#include "thor.h"
#include "grf.h"
#include <zlib.h>
#include <cstring>
#include <cstdio>
#include <Windows.h>

namespace autopatch
{

    // THOR file signatures
    static const char THOR_SIGNATURE[] = "ASSF (C) 2007 Aeomin DEV";
    static const char THOR_SIGNATURE_LEGACY[] = "ASSF (C) 2007 Aeokan (aeokan@gmail.com)";
    static const size_t MAGIC_SIZE = 24;
    static const size_t MAGIC_LEGACY_SIZE = 48;

    // Mode constants
    static const uint16_t MODE_SINGLE_FILE = 0x21;    // 33 decimal
    static const uint16_t MODE_MULTIPLE_FILES = 0x30; // 48 decimal

    // Entry flags
    static const uint8_t ENTRY_FLAG_REMOVE = 0x01;

    ThorFile::ThorFile() = default;

    ThorFile::~ThorFile()
    {
        Close();
    }

    bool ThorFile::Open(const std::wstring &path)
    {
        Close();

        OutputDebugStringW((L"[THOR] Abrindo arquivo: " + path + L"\n").c_str());

        m_file.open(path, std::ios::binary);
        if (!m_file.is_open())
        {
            OutputDebugStringW(L"[THOR] ERRO: Não foi possível abrir o arquivo\n");
            return false;
        }

        m_path = path;

        if (!ReadHeader())
        {
            OutputDebugStringW(L"[THOR] ERRO: Falha ao ler header\n");
            Close();
            return false;
        }

        if (!ReadFileTable())
        {
            OutputDebugStringW(L"[THOR] ERRO: Falha ao ler tabela de arquivos\n");
            Close();
            return false;
        }

        OutputDebugStringW((L"[THOR] Arquivo aberto com sucesso. Arquivos: " +
                            std::to_wstring(m_fileCount) + L"\n")
                               .c_str());
        m_isOpen = true;
        return true;
    }

    void ThorFile::Close()
    {
        if (m_file.is_open())
        {
            m_file.close();
        }

        m_isOpen = false;
        m_mode = ThorMode::Invalid;
        m_targetGrf.clear();
        m_fileCount = 0;
        m_entries.clear();
        m_dataStartOffset = 0;
    }

    bool ThorFile::ReadHeader()
    {
        // Ler 48 bytes para verificar qual formato é
        char magic[48];
        m_file.read(magic, 48);

        // Debug: mostra assinatura lida
        OutputDebugStringA("[THOR] Assinatura lida (primeiros 24 bytes): ");
        OutputDebugStringA(std::string(magic, 24).c_str());
        OutputDebugStringA("\n");

        // Verificar formato GRF Editor (24 bytes)
        if (memcmp(magic, THOR_SIGNATURE, MAGIC_SIZE) == 0)
        {
            OutputDebugStringW(L"[THOR] Formato GRF Editor detectado (24 bytes magic)\n");
            // Voltar para posição após os 24 bytes do magic
            m_file.seekg(MAGIC_SIZE);
        }
        // Verificar formato Thor Patcher legado (48 bytes)
        else if (memcmp(magic, THOR_SIGNATURE_LEGACY, MAGIC_LEGACY_SIZE) == 0)
        {
            OutputDebugStringW(L"[THOR] Formato Thor Patcher legado detectado (48 bytes magic)\n");
            // Já estamos na posição correta (após os 48 bytes)
        }
        else
        {
            OutputDebugStringW(L"[THOR] Assinatura não corresponde a nenhum formato conhecido\n");

            OutputDebugStringA("[THOR] Primeiros 48 bytes em hex: ");
            for (int i = 0; i < 48; i++)
            {
                char hexBuf[8];
                sprintf_s(hexBuf, "%02X ", (unsigned char)magic[i]);
                OutputDebugStringA(hexBuf);
            }
            OutputDebugStringA("\n");

            return false;
        }

        // Ler UseGrfMerging (1 byte)
        uint8_t useGrfMerging;
        m_file.read(reinterpret_cast<char *>(&useGrfMerging), 1);
        m_useGrfMerging = (useGrfMerging == 1);
        OutputDebugStringW((L"[THOR] UseGrfMerging: " + std::to_wstring(useGrfMerging) +
                            L" (" + (m_useGrfMerging ? L"GRF merge" : L"Disk extract") + L")\n")
                               .c_str());

        // Ler NumberOfFiles (4 bytes)
        m_file.read(reinterpret_cast<char *>(&m_fileCount), 4);
        OutputDebugStringW((L"[THOR] Número de arquivos: " + std::to_wstring(m_fileCount) + L"\n").c_str());

        // Ler Mode (2 bytes)
        uint16_t mode;
        m_file.read(reinterpret_cast<char *>(&mode), 2);
        OutputDebugStringW((L"[THOR] Mode: 0x" + std::to_wstring(mode) + L" (" + std::to_wstring(mode) + L")\n").c_str());

        // Ler TargetGrfLength (1 byte) e TargetGrf (N bytes)
        uint8_t grfNameLen;
        m_file.read(reinterpret_cast<char *>(&grfNameLen), 1);

        if (grfNameLen > 0)
        {
            m_targetGrf.resize(grfNameLen);
            m_file.read(m_targetGrf.data(), grfNameLen);
            OutputDebugStringA("[THOR] GRF alvo: ");
            OutputDebugStringA(m_targetGrf.c_str());
            OutputDebugStringA("\n");
        }

        if (mode == MODE_MULTIPLE_FILES) // 0x30 = 48 = modo múltiplos arquivos (GRF Editor)
        {
            OutputDebugStringW(L"[THOR] Modo: MULTIPLE_FILES (GRF Editor format)\n");
            m_mode = ThorMode::MultiFile;

            // FileTableCompLen (4 bytes)
            m_file.read(reinterpret_cast<char *>(&m_fileTableCompLen), 4);

            // FileTableOffset (4 bytes)
            m_file.read(reinterpret_cast<char *>(&m_fileTableOffset), 4);

            // Salvar posição de início dos dados (logo após os metadados do header)
            m_dataStartOffset = static_cast<uint32_t>(m_file.tellg());

            OutputDebugStringW((L"[THOR] Table comp len: " + std::to_wstring(m_fileTableCompLen) + L"\n").c_str());
            OutputDebugStringW((L"[THOR] Table offset: " + std::to_wstring(m_fileTableOffset) + L"\n").c_str());
            OutputDebugStringW((L"[THOR] Data start offset: " + std::to_wstring(m_dataStartOffset) + L"\n").c_str());
        }
        else if (mode == MODE_SINGLE_FILE) // 0x21 = 33 = modo arquivo único
        {
            OutputDebugStringW(L"[THOR] Modo: SINGLE_FILE\n");
            m_mode = ThorMode::SingleFile;

            // FileTableOffset (8 bytes para single file)
            uint64_t tableOffset64;
            m_file.read(reinterpret_cast<char *>(&tableOffset64), 8);
            m_fileTableOffset = static_cast<uint32_t>(tableOffset64);
            m_dataStartOffset = static_cast<uint32_t>(m_file.tellg());

            OutputDebugStringW((L"[THOR] Table offset (64-bit): " + std::to_wstring(tableOffset64) + L"\n").c_str());
        }
        else
        {
            OutputDebugStringW((L"[THOR] Modo desconhecido: 0x" + std::to_wstring(mode) + L"\n").c_str());
            return false;
        }

        return m_file.good();
    }

    bool ThorFile::ReadFileTable()
    {
        m_entries.clear();
        m_entries.reserve(m_fileCount);

        if (m_mode == ThorMode::MultiFile)
        {
            return ReadMultipleFilesTable();
        }
        else if (m_mode == ThorMode::SingleFile)
        {
            return ReadSingleFileTable();
        }

        return false;
    }

    bool ThorFile::ReadSingleFileTable()
    {
        OutputDebugStringW(L"[THOR] Lendo tabela de arquivo único\n");

        // Ir para a tabela
        m_file.seekg(m_fileTableOffset);

        ThorEntry entry;

        // Ler tamanho do nome (1 byte)
        uint8_t nameLen;
        m_file.read(reinterpret_cast<char *>(&nameLen), 1);

        // Ler nome
        entry.filename.resize(nameLen);
        m_file.read(entry.filename.data(), nameLen);

        // Ler flags (1 byte)
        m_file.read(reinterpret_cast<char *>(&entry.flags), 1);

        // Ler offset dos dados (8 bytes)
        uint64_t offset64;
        m_file.read(reinterpret_cast<char *>(&offset64), 8);
        entry.offset = static_cast<uint32_t>(offset64);

        // Ler tamanho comprimido (4 bytes)
        m_file.read(reinterpret_cast<char *>(&entry.compressedSize), 4);

        // Ler tamanho descomprimido (4 bytes)
        m_file.read(reinterpret_cast<char *>(&entry.uncompressedSize), 4);

        OutputDebugStringA("[THOR] Single file: ");
        OutputDebugStringA(entry.filename.c_str());
        OutputDebugStringA("\n");

        m_entries.push_back(std::move(entry));

        return m_file.good();
    }

    bool ThorFile::ReadMultipleFilesTable()
    {
        OutputDebugStringW(L"[THOR] Lendo tabela de múltiplos arquivos (comprimida)\n");

        // Ir para a tabela
        m_file.seekg(m_fileTableOffset);

        // Ler tabela comprimida
        std::vector<uint8_t> compressedTable(m_fileTableCompLen);
        m_file.read(reinterpret_cast<char *>(compressedTable.data()), m_fileTableCompLen);

        if (!m_file)
        {
            OutputDebugStringW(L"[THOR] ERRO: Falha ao ler tabela comprimida\n");
            return false;
        }

        OutputDebugStringW((L"[THOR] Tabela comprimida lida: " + std::to_wstring(m_fileTableCompLen) + L" bytes\n").c_str());

        // Estimar tamanho descomprimido (aproximadamente 10x)
        size_t estimatedSize = m_fileTableCompLen * 20;
        std::vector<uint8_t> table(estimatedSize);

        // Tentar descomprimir com raw deflate (sem header zlib)
        z_stream strm = {};
        strm.next_in = compressedTable.data();
        strm.avail_in = static_cast<uInt>(compressedTable.size());
        strm.next_out = table.data();
        strm.avail_out = static_cast<uInt>(table.size());

        // -MAX_WBITS = raw deflate (sem header zlib), como .NET DeflateStream
        int ret = inflateInit2(&strm, -MAX_WBITS);
        if (ret != Z_OK)
        {
            OutputDebugStringW(L"[THOR] ERRO: inflateInit2 falhou\n");
            return false;
        }

        ret = inflate(&strm, Z_FINISH);
        size_t decompressedSize = strm.total_out;
        inflateEnd(&strm);

        if (ret != Z_STREAM_END && ret != Z_OK)
        {
            OutputDebugStringW((L"[THOR] Raw deflate falhou (ret=" + std::to_wstring(ret) + L"), tentando zlib\n").c_str());

            // Tentar com zlib normal (com header)
            uLongf destLen = static_cast<uLongf>(estimatedSize);
            if (uncompress(table.data(), &destLen, compressedTable.data(), static_cast<uLong>(compressedTable.size())) != Z_OK)
            {
                OutputDebugStringW(L"[THOR] ERRO: Falha ao descomprimir tabela\n");
                return false;
            }
            decompressedSize = destLen;
        }

        table.resize(decompressedSize);
        OutputDebugStringW((L"[THOR] Tabela descomprimida: " + std::to_wstring(decompressedSize) + L" bytes\n").c_str());

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
        for (uint32_t i = 0; i < m_fileCount; i++)
        {
            if (pos >= table.size())
            {
                OutputDebugStringW((L"[THOR] Fim prematuro da tabela na entrada " + std::to_wstring(i) + L"\n").c_str());
                break;
            }

            ThorEntry entry;

            // Ler tamanho do nome
            uint8_t nameLen = table[pos++];
            if (pos + nameLen > table.size())
                break;

            entry.filename.assign(reinterpret_cast<char *>(&table[pos]), nameLen);
            pos += nameLen;

            // Ler flags
            if (pos >= table.size())
                break;
            entry.flags = table[pos++];

            if ((entry.flags & ENTRY_FLAG_REMOVE) == 0)
            {
                // Ler offset (4 bytes)
                if (pos + 4 > table.size())
                    break;
                memcpy(&entry.offset, &table[pos], 4);
                // O offset é relativo ao início dos dados, não ao início do arquivo
                // Mas na implementação de referência, o offset já é absoluto
                pos += 4;

                // Ler tamanho comprimido (4 bytes)
                if (pos + 4 > table.size())
                    break;
                memcpy(&entry.compressedSize, &table[pos], 4);
                pos += 4;

                // Ler tamanho descomprimido (4 bytes)
                if (pos + 4 > table.size())
                    break;
                memcpy(&entry.uncompressedSize, &table[pos], 4);
                pos += 4;
            }
            else
            {
                entry.offset = 0;
                entry.compressedSize = 0;
                entry.uncompressedSize = 0;
            }

            OutputDebugStringA("[THOR] Entry: ");
            OutputDebugStringA(entry.filename.c_str());
            OutputDebugStringA((entry.flags & ENTRY_FLAG_REMOVE) ? " [REMOVE]" : "");
            OutputDebugStringA("\n");

            m_entries.push_back(std::move(entry));
        }

        OutputDebugStringW((L"[THOR] Total de entradas lidas: " + std::to_wstring(m_entries.size()) + L"\n").c_str());
        return true;
    }

    std::vector<uint8_t> ThorFile::ExtractFile(const ThorEntry &entry)
    {
        if ((entry.flags & ENTRY_FLAG_REMOVE) != 0)
        {
            // Arquivo marcado para deleção
            return {};
        }

        OutputDebugStringA("[THOR] Extraindo: ");
        OutputDebugStringA(entry.filename.c_str());
        OutputDebugStringA("\n");
        OutputDebugStringW((L"[THOR] Offset: " + std::to_wstring(entry.offset) +
                            L", CompSize: " + std::to_wstring(entry.compressedSize) +
                            L", Size: " + std::to_wstring(entry.uncompressedSize) + L"\n")
                               .c_str());

        // Posiciona no offset (os offsets no THOR já são absolutos)
        m_file.seekg(entry.offset);

        // Lê dados comprimidos
        std::vector<uint8_t> compressed(entry.compressedSize);
        m_file.read(reinterpret_cast<char *>(compressed.data()), entry.compressedSize);

        if (!m_file)
        {
            OutputDebugStringW(L"[THOR] ERRO: Falha ao ler dados do arquivo\n");
            return {};
        }

        // Se não está comprimido
        if (entry.compressedSize == entry.uncompressedSize)
        {
            OutputDebugStringW(L"[THOR] Arquivo não comprimido\n");
            return compressed;
        }

        // Descomprime - tentar raw deflate primeiro (formato .NET DeflateStream)
        std::vector<uint8_t> uncompressed(entry.uncompressedSize);

        z_stream strm = {};
        strm.next_in = compressed.data();
        strm.avail_in = static_cast<uInt>(compressed.size());
        strm.next_out = uncompressed.data();
        strm.avail_out = static_cast<uInt>(uncompressed.size());

        int ret = inflateInit2(&strm, -MAX_WBITS);
        if (ret == Z_OK)
        {
            ret = inflate(&strm, Z_FINISH);
            inflateEnd(&strm);

            if (ret == Z_STREAM_END)
            {
                OutputDebugStringW(L"[THOR] Descomprimido com raw deflate\n");
                return uncompressed;
            }
        }

        // Tentar zlib normal
        uLongf destLen = entry.uncompressedSize;
        if (uncompress(uncompressed.data(), &destLen, compressed.data(), entry.compressedSize) == Z_OK)
        {
            OutputDebugStringW(L"[THOR] Descomprimido com zlib\n");
            uncompressed.resize(destLen);
            return uncompressed;
        }

        OutputDebugStringW(L"[THOR] ERRO: Falha ao descomprimir dados\n");
        return {};
    }

    bool ThorFile::ApplyTo(GrfFile &grf)
    {
        if (!m_isOpen || !grf.IsOpen())
        {
            OutputDebugStringW(L"[THOR] ERRO: Arquivo THOR ou GRF não está aberto\n");
            return false;
        }

        OutputDebugStringW((L"[THOR] Aplicando " + std::to_wstring(m_entries.size()) + L" arquivos ao GRF\n").c_str());

        for (const auto &entry : m_entries)
        {
            if ((entry.flags & ENTRY_FLAG_REMOVE) != 0)
            {
                // Remove arquivo
                OutputDebugStringA("[THOR] Removendo do GRF: ");
                OutputDebugStringA(entry.filename.c_str());
                OutputDebugStringA("\n");
                grf.RemoveFile(entry.filename);
            }
            else
            {
                // Adiciona/atualiza arquivo
                auto data = ExtractFile(entry);
                if (!data.empty())
                {
                    OutputDebugStringA("[THOR] Adicionando ao GRF: ");
                    OutputDebugStringA(entry.filename.c_str());
                    OutputDebugStringA("\n");
                    grf.AddFile(entry.filename, data);
                }
                else
                {
                    OutputDebugStringA("[THOR] ERRO: Dados vazios para: ");
                    OutputDebugStringA(entry.filename.c_str());
                    OutputDebugStringA("\n");
                }
            }
        }

        return true;
    }

    bool ThorFile::ApplyToDisk(const std::wstring &outputDir)
    {
        if (!m_isOpen)
        {
            OutputDebugStringW(L"[THOR] ERRO: Arquivo THOR não está aberto\n");
            return false;
        }

        OutputDebugStringW((L"[THOR] Extraindo " + std::to_wstring(m_entries.size()) +
                            L" arquivos para: " + outputDir + L"\n")
                               .c_str());

        // Garante que outputDir termina com barra
        std::wstring baseDir = outputDir;
        if (!baseDir.empty() && baseDir.back() != L'\\' && baseDir.back() != L'/')
        {
            baseDir += L'\\';
        }

        for (const auto &entry : m_entries)
        {
            // Converte caminho do arquivo para wide string
            std::wstring relativePath;
            for (char c : entry.filename)
            {
                if (c == '/')
                    relativePath += L'\\';
                else
                    relativePath += static_cast<wchar_t>(c);
            }

            std::wstring fullPath = baseDir + relativePath;

            if ((entry.flags & ENTRY_FLAG_REMOVE) != 0)
            {
                // Remove arquivo do disco
                OutputDebugStringW((L"[THOR] Removendo do disco: " + fullPath + L"\n").c_str());
                DeleteFileW(fullPath.c_str());
            }
            else
            {
                // Extrai arquivo para disco
                auto data = ExtractFile(entry);
                if (!data.empty())
                {
                    OutputDebugStringW((L"[THOR] Extraindo para: " + fullPath + L"\n").c_str());

                    // Cria diretórios pai se necessário
                    size_t pos = fullPath.find_last_of(L"\\/");
                    if (pos != std::wstring::npos)
                    {
                        std::wstring dir = fullPath.substr(0, pos);
                        // Cria recursivamente
                        size_t p = 0;
                        while ((p = dir.find_first_of(L"\\/", p + 1)) != std::wstring::npos)
                        {
                            CreateDirectoryW(dir.substr(0, p).c_str(), nullptr);
                        }
                        CreateDirectoryW(dir.c_str(), nullptr);
                    }

                    // Escreve arquivo
                    HANDLE hFile = CreateFileW(fullPath.c_str(), GENERIC_WRITE, 0, nullptr,
                                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                    if (hFile != INVALID_HANDLE_VALUE)
                    {
                        DWORD written;
                        WriteFile(hFile, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);
                        CloseHandle(hFile);
                    }
                    else
                    {
                        OutputDebugStringW((L"[THOR] ERRO: Não foi possível criar: " + fullPath + L"\n").c_str());
                    }
                }
                else
                {
                    OutputDebugStringA("[THOR] ERRO: Dados vazios para: ");
                    OutputDebugStringA(entry.filename.c_str());
                    OutputDebugStringA("\n");
                }
            }
        }

        return true;
    }

} // namespace autopatch
