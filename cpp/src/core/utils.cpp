#include "utils.h"
#include <zlib.h>
#include <wincrypt.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <shellapi.h>
#include <VersionHelpers.h>
#include <fstream>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Advapi32.lib")

namespace autopatch
{
    namespace utils
    {

        // ============================================================================
        // Conversão de strings
        // ============================================================================

        std::wstring StringToWide(const std::string &str)
        {
            if (str.empty())
                return L"";

            int size = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
            if (size <= 0)
                return L"";

            std::wstring result(size - 1, 0);
            MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, result.data(), size);

            return result;
        }

        std::string WideToString(const std::wstring &wstr)
        {
            if (wstr.empty())
                return "";

            int size = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (size <= 0)
                return "";

            std::string result(size - 1, 0);
            WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, result.data(), size, nullptr, nullptr);

            return result;
        }

        std::string WideToUtf8(const std::wstring &wstr)
        {
            if (wstr.empty())
                return "";

            int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (size <= 0)
                return "";

            std::string result(size - 1, 0);
            WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, result.data(), size, nullptr, nullptr);

            return result;
        }

        std::wstring Utf8ToWide(const std::string &utf8)
        {
            if (utf8.empty())
                return L"";

            int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
            if (size <= 0)
                return L"";

            std::wstring result(size - 1, 0);
            MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, result.data(), size);

            return result;
        }

        // ============================================================================
        // Manipulação de arquivos
        // ============================================================================

        bool FileExists(const std::wstring &path)
        {
            DWORD attrib = GetFileAttributesW(path.c_str());
            return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
        }

        bool DirectoryExists(const std::wstring &path)
        {
            DWORD attrib = GetFileAttributesW(path.c_str());
            return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
        }

        bool CreateDirectoryRecursive(const std::wstring &path)
        {
            // Usa SHCreateDirectoryEx para criar todos os diretórios
            int result = SHCreateDirectoryExW(nullptr, path.c_str(), nullptr);
            return result == ERROR_SUCCESS || result == ERROR_ALREADY_EXISTS;
        }

        bool DeleteFileW(const std::wstring &path)
        {
            return ::DeleteFileW(path.c_str()) != 0;
        }

        uint64_t GetFileSize(const std::wstring &path)
        {
            WIN32_FILE_ATTRIBUTE_DATA fileInfo;
            if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fileInfo))
            {
                LARGE_INTEGER size;
                size.HighPart = fileInfo.nFileSizeHigh;
                size.LowPart = fileInfo.nFileSizeLow;
                return size.QuadPart;
            }
            return 0;
        }

        std::wstring GetTempDirectory()
        {
            wchar_t buffer[MAX_PATH];
            GetTempPathW(MAX_PATH, buffer);
            return buffer;
        }

        std::wstring GetAppDirectory()
        {
            wchar_t buffer[MAX_PATH];
            GetModuleFileNameW(nullptr, buffer, MAX_PATH);

            std::wstring path(buffer);
            size_t pos = path.find_last_of(L"\\/");
            if (pos != std::wstring::npos)
            {
                path = path.substr(0, pos);
            }

            return path;
        }

        std::wstring GetFileName(const std::wstring &path)
        {
            size_t pos = path.find_last_of(L"\\/");
            if (pos != std::wstring::npos)
            {
                return path.substr(pos + 1);
            }
            return path;
        }

        std::wstring GetFileExtension(const std::wstring &path)
        {
            size_t pos = path.find_last_of(L'.');
            if (pos != std::wstring::npos)
            {
                return path.substr(pos);
            }
            return L"";
        }

        std::wstring GetDirectoryPath(const std::wstring &path)
        {
            size_t pos = path.find_last_of(L"\\/");
            if (pos != std::wstring::npos)
            {
                return path.substr(0, pos);
            }
            return L"";
        }

        std::wstring CombinePath(const std::wstring &base, const std::wstring &relative)
        {
            if (base.empty())
                return relative;
            if (relative.empty())
                return base;

            std::wstring result = base;
            if (result.back() != L'\\' && result.back() != L'/')
            {
                result += L'\\';
            }

            // Remove leading slash from relative
            size_t start = 0;
            while (start < relative.size() && (relative[start] == L'\\' || relative[start] == L'/'))
            {
                start++;
            }

            result += relative.substr(start);
            return result;
        }

        std::wstring NormalizePath(const std::wstring &path)
        {
            std::wstring result = path;

            // Substitui / por \ 
    for (auto& c : result) {
            if (c == L'/')
                c = L'\\';
        }

        // Remove barras duplicadas
        size_t pos = 0;
        while ((pos = result.find(L"\\\\", pos)) != std::wstring::npos)
        {
            result.erase(pos, 1);
        }

        // Remove barra final (exceto para raiz)
        if (result.size() > 3 && result.back() == L'\\')
        {
            result.pop_back();
        }

        return result;
    }

    // ============================================================================
    // Leitura/escrita de arquivos
    // ============================================================================

    std::vector<uint8_t> ReadAllBytes(const std::wstring &path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return {};
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(size);
        file.read(reinterpret_cast<char *>(buffer.data()), size);

        return buffer;
    }

    bool WriteAllBytes(const std::wstring &path, const std::vector<uint8_t> &data)
    {
        // Cria diretório se necessário
        std::wstring dir = GetDirectoryPath(path);
        if (!dir.empty() && !DirectoryExists(dir))
        {
            CreateDirectoryRecursive(dir);
        }

        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        file.write(reinterpret_cast<const char *>(data.data()), data.size());
        return file.good();
    }

    std::string ReadAllText(const std::wstring &path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    bool WriteAllText(const std::wstring &path, const std::string &text)
    {
        // Cria diretório se necessário
        std::wstring dir = GetDirectoryPath(path);
        if (!dir.empty() && !DirectoryExists(dir))
        {
            CreateDirectoryRecursive(dir);
        }

        std::ofstream file(path);
        if (!file.is_open())
        {
            return false;
        }

        file << text;
        return file.good();
    }

    // ============================================================================
    // Hash/CRC
    // ============================================================================

    uint32_t Crc32(const void *data, size_t size)
    {
        uLong crc = crc32(0L, Z_NULL, 0);
        const Bytef *ptr = static_cast<const Bytef *>(data);
        size_t remaining = size;
        const size_t CHUNK_SIZE = 0x40000000; // 1GB

        while (remaining > 0)
        {
            uInt chunk = (remaining > CHUNK_SIZE) ? static_cast<uInt>(CHUNK_SIZE) : static_cast<uInt>(remaining);
            crc = crc32(crc, ptr, chunk);
            ptr += chunk;
            remaining -= chunk;
        }

        return static_cast<uint32_t>(crc);
    }

    uint32_t Crc32File(const std::wstring &path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            return 0;
        }

        constexpr size_t bufferSize = 256 * 1024; // 256KB
        std::vector<char> buffer(bufferSize);
        uLong crc = crc32(0L, Z_NULL, 0);

        while (file)
        {
            file.read(buffer.data(), bufferSize);
            std::streamsize count = file.gcount();
            if (count > 0)
            {
                crc = crc32(crc, reinterpret_cast<const Bytef *>(buffer.data()), static_cast<uInt>(count));
            }
        }

        return static_cast<uint32_t>(crc);
    }

    std::string Md5(const void *data, size_t size)
    {
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        std::string result;

        if (!CryptAcquireContextW(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        {
            return "";
        }

        if (CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
        {
            if (CryptHashData(hHash, static_cast<const BYTE *>(data), static_cast<DWORD>(size), 0))
            {
                BYTE hash[16];
                DWORD hashLen = 16;

                if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0))
                {
                    std::stringstream ss;
                    for (int i = 0; i < 16; i++)
                    {
                        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
                    }
                    result = ss.str();
                }
            }
            CryptDestroyHash(hHash);
        }

        CryptReleaseContext(hProv, 0);
        return result;
    }

    std::string Md5File(const std::wstring &path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            return "";
        }

        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        std::string result;

        if (!CryptAcquireContextW(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        {
            return "";
        }

        if (CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
        {
            constexpr size_t bufferSize = 256 * 1024; // 256KB
            std::vector<char> buffer(bufferSize);
            bool success = true;

            while (file)
            {
                file.read(buffer.data(), bufferSize);
                std::streamsize count = file.gcount();
                if (count > 0)
                {
                    if (!CryptHashData(hHash, reinterpret_cast<const BYTE *>(buffer.data()), static_cast<DWORD>(count), 0))
                    {
                        success = false;
                        break;
                    }
                }
            }

            if (success)
            {
                BYTE hash[16];
                DWORD hashLen = 16;

                if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0))
                {
                    std::stringstream ss;
                    for (int i = 0; i < 16; i++)
                    {
                        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
                    }
                    result = ss.str();
                }
            }
            CryptDestroyHash(hHash);
        }

        CryptReleaseContext(hProv, 0);
        return result;
    }

    // ============================================================================
    // Compressão
    // ============================================================================

    std::vector<uint8_t> Compress(const std::vector<uint8_t> &data)
    {
        uLongf compressedSize = compressBound(static_cast<uLong>(data.size()));
        std::vector<uint8_t> compressed(compressedSize);

        if (compress(compressed.data(), &compressedSize, data.data(), static_cast<uLong>(data.size())) != Z_OK)
        {
            return {};
        }

        compressed.resize(compressedSize);
        return compressed;
    }

    std::vector<uint8_t> Decompress(const std::vector<uint8_t> &data, size_t uncompressedSize)
    {
        std::vector<uint8_t> decompressed(uncompressedSize);
        uLongf destLen = static_cast<uLongf>(uncompressedSize);

        if (uncompress(decompressed.data(), &destLen, data.data(), static_cast<uLong>(data.size())) != Z_OK)
        {
            return {};
        }

        decompressed.resize(destLen);
        return decompressed;
    }

    // ============================================================================
    // Base64
    // ============================================================================

    static const char *base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string Base64Encode(const std::vector<uint8_t> &data)
    {
        std::string result;
        result.reserve(((data.size() + 2) / 3) * 4);

        for (size_t i = 0; i < data.size(); i += 3)
        {
            uint32_t octet_a = data[i];
            uint32_t octet_b = i + 1 < data.size() ? data[i + 1] : 0;
            uint32_t octet_c = i + 2 < data.size() ? data[i + 2] : 0;

            uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

            result += base64_chars[(triple >> 18) & 0x3F];
            result += base64_chars[(triple >> 12) & 0x3F];
            result += i + 1 < data.size() ? base64_chars[(triple >> 6) & 0x3F] : '=';
            result += i + 2 < data.size() ? base64_chars[triple & 0x3F] : '=';
        }

        return result;
    }

    std::vector<uint8_t> Base64Decode(const std::string &encoded)
    {
        static int decoding_table[256] = {-1};
        static bool table_initialized = false;

        if (!table_initialized)
        {
            for (int i = 0; i < 256; i++)
                decoding_table[i] = -1;
            for (int i = 0; i < 64; i++)
                decoding_table[(unsigned char)base64_chars[i]] = i;
            table_initialized = true;
        }

        size_t padding = 0;
        if (encoded.size() >= 2)
        {
            if (encoded[encoded.size() - 1] == '=')
                padding++;
            if (encoded[encoded.size() - 2] == '=')
                padding++;
        }

        std::vector<uint8_t> result;
        result.reserve((encoded.size() / 4) * 3 - padding);

        uint32_t accumulator = 0;
        int bits_collected = 0;

        for (char c : encoded)
        {
            if (c == '=')
                break;

            int value = decoding_table[(unsigned char)c];
            if (value < 0)
                continue;

            accumulator = (accumulator << 6) | value;
            bits_collected += 6;

            if (bits_collected >= 8)
            {
                bits_collected -= 8;
                result.push_back(static_cast<uint8_t>((accumulator >> bits_collected) & 0xFF));
            }
        }

        return result;
    }

    // ============================================================================
    // Formatação
    // ============================================================================

    std::wstring FormatFileSize(uint64_t bytes)
    {
        const wchar_t *units[] = {L"B", L"KB", L"MB", L"GB", L"TB"};
        int unit = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024 && unit < 4)
        {
            size /= 1024;
            unit++;
        }

        wchar_t buffer[64];
        if (unit == 0)
        {
            swprintf(buffer, 64, L"%.0f %s", size, units[unit]);
        }
        else
        {
            swprintf(buffer, 64, L"%.2f %s", size, units[unit]);
        }

        return buffer;
    }

    std::wstring FormatSpeed(double bytesPerSecond)
    {
        return FormatFileSize(static_cast<uint64_t>(bytesPerSecond)) + L"/s";
    }

    std::wstring FormatTime(int seconds)
    {
        if (seconds < 60)
        {
            return std::to_wstring(seconds) + L"s";
        }

        int minutes = seconds / 60;
        seconds = seconds % 60;

        if (minutes < 60)
        {
            wchar_t buffer[32];
            swprintf(buffer, 32, L"%dm %02ds", minutes, seconds);
            return buffer;
        }

        int hours = minutes / 60;
        minutes = minutes % 60;

        wchar_t buffer[32];
        swprintf(buffer, 32, L"%dh %02dm %02ds", hours, minutes, seconds);
        return buffer;
    }

    // ============================================================================
    // Sistema
    // ============================================================================

    bool IsWindows10OrLater()
    {
        return IsWindows10OrGreater();
    }

    std::wstring GetWindowsVersion()
    {
        OSVERSIONINFOEXW osvi = {0};
        osvi.dwOSVersionInfoSize = sizeof(osvi);

        // Use RtlGetVersion para obter versão real
        typedef NTSTATUS(WINAPI * RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
        HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
        if (hNtdll)
        {
            auto RtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(hNtdll, "RtlGetVersion"));
            if (RtlGetVersion)
            {
                RtlGetVersion(reinterpret_cast<PRTL_OSVERSIONINFOW>(&osvi));
            }
        }

        wchar_t buffer[64];
        swprintf(buffer, 64, L"Windows %d.%d.%d", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
        return buffer;
    }

    bool IsElevated()
    {
        BOOL elevated = FALSE;
        HANDLE hToken = nullptr;

        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            TOKEN_ELEVATION elevation;
            DWORD size;

            if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size))
            {
                elevated = elevation.TokenIsElevated;
            }

            CloseHandle(hToken);
        }

        return elevated != FALSE;
    }

    // ============================================================================
    // Processos
    // ============================================================================

    bool RunProcess(const std::wstring &path, const std::wstring &args, bool wait)
    {
        SHELLEXECUTEINFOW sei = {0};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb = L"open";
        sei.lpFile = path.c_str();
        sei.lpParameters = args.empty() ? nullptr : args.c_str();
        sei.nShow = SW_SHOW;

        if (!ShellExecuteExW(&sei))
        {
            return false;
        }

        if (wait && sei.hProcess)
        {
            WaitForSingleObject(sei.hProcess, INFINITE);
            CloseHandle(sei.hProcess);
        }

        return true;
    }

    bool ShellExecuteFile(const std::wstring &path)
    {
        HINSTANCE result = ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOW);
        return reinterpret_cast<INT_PTR>(result) > 32;
    }

} // namespace utils
} // namespace autopatch
