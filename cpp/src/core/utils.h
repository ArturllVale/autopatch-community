#pragma once

#include <string>
#include <vector>
#include <functional>
#include <Windows.h>

namespace autopatch
{
    namespace utils
    {

        // Conversão de strings
        std::wstring StringToWide(const std::string &str);
        std::string WideToString(const std::wstring &wstr);
        std::string WideToUtf8(const std::wstring &wstr);
        std::wstring Utf8ToWide(const std::string &utf8);

        // Manipulação de arquivos
        bool FileExists(const std::wstring &path);
        bool DirectoryExists(const std::wstring &path);
        bool CreateDirectoryRecursive(const std::wstring &path);
        bool DeleteFileW(const std::wstring &path);
        uint64_t GetFileSize(const std::wstring &path);
        std::wstring GetTempDirectory();
        std::wstring GetAppDirectory();
        std::wstring GetFileName(const std::wstring &path);
        std::wstring GetFileExtension(const std::wstring &path);
        std::wstring GetDirectoryPath(const std::wstring &path);
        std::wstring CombinePath(const std::wstring &base, const std::wstring &relative);
        std::wstring NormalizePath(const std::wstring &path);

        // Leitura/escrita de arquivos
        std::vector<uint8_t> ReadAllBytes(const std::wstring &path);
        bool WriteAllBytes(const std::wstring &path, const std::vector<uint8_t> &data);
        std::string ReadAllText(const std::wstring &path);
        bool WriteAllText(const std::wstring &path, const std::string &text);

        // Hash/CRC
        uint32_t Crc32(const void *data, size_t size);
        uint32_t Crc32File(const std::wstring &path);
        std::string Md5(const void *data, size_t size);
        std::string Md5File(const std::wstring &path);

        // Compressão
        std::vector<uint8_t> Compress(const std::vector<uint8_t> &data);
        std::vector<uint8_t> Decompress(const std::vector<uint8_t> &data, size_t uncompressedSize);

        // Base64
        std::string Base64Encode(const std::vector<uint8_t> &data);
        std::vector<uint8_t> Base64Decode(const std::string &encoded);

        // Formatação
        std::wstring FormatFileSize(uint64_t bytes);
        std::wstring FormatSpeed(double bytesPerSecond);
        std::wstring FormatTime(int seconds);

        // Sistema
        bool IsWindows10OrLater();
        std::wstring GetWindowsVersion();
        bool IsElevated();

        // Processos
        bool RunProcess(const std::wstring &path, const std::wstring &args = L"", bool wait = false);
        bool ShellExecuteFile(const std::wstring &path);

    } // namespace utils
} // namespace autopatch
