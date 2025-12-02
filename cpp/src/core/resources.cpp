#include "resources.h"

namespace autopatch
{

    std::vector<uint8_t> Resources::LoadRCData(int resourceId, HMODULE hModule)
    {
        if (hModule == nullptr)
        {
            hModule = GetModuleHandle(nullptr);
        }

        HRSRC hResInfo = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (!hResInfo)
        {
            return {};
        }

        DWORD size = SizeofResource(hModule, hResInfo);
        if (size == 0)
        {
            return {};
        }

        HGLOBAL hResData = LoadResource(hModule, hResInfo);
        if (!hResData)
        {
            return {};
        }

        void *pData = LockResource(hResData);
        if (!pData)
        {
            return {};
        }

        std::vector<uint8_t> result(size);
        memcpy(result.data(), pData, size);
        return result;
    }

    std::vector<uint8_t> Resources::LoadPNG(int resourceId, HMODULE hModule)
    {
        return LoadRCData(resourceId, hModule);
    }

    std::wstring Resources::LoadString(int resourceId, HMODULE hModule)
    {
        if (hModule == nullptr)
        {
            hModule = GetModuleHandle(nullptr);
        }

        wchar_t buffer[4096];
        int len = ::LoadStringW(hModule, resourceId, buffer, 4096);
        if (len > 0)
        {
            return std::wstring(buffer, len);
        }
        return {};
    }

    bool Resources::EmbedRCData(const std::wstring &exePath, int resourceId,
                                const std::vector<uint8_t> &data)
    {
        HANDLE hUpdate = BeginUpdateResourceW(exePath.c_str(), FALSE);
        if (!hUpdate)
        {
            return false;
        }

        BOOL success = UpdateResourceW(
            hUpdate,
            RT_RCDATA,
            MAKEINTRESOURCE(resourceId),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            (LPVOID)data.data(),
            static_cast<DWORD>(data.size()));

        if (!success)
        {
            EndUpdateResourceW(hUpdate, TRUE); // Discard
            return false;
        }

        return EndUpdateResourceW(hUpdate, FALSE) != FALSE;
    }

    bool Resources::EmbedRCData(const std::wstring &exePath, int resourceId,
                                const std::string &data)
    {
        std::vector<uint8_t> bytes(data.begin(), data.end());
        return EmbedRCData(exePath, resourceId, bytes);
    }

    bool Resources::EmbedMultipleResources(const std::wstring &exePath,
                                           const std::vector<std::pair<int, std::vector<uint8_t>>> &resources)
    {

        HANDLE hUpdate = BeginUpdateResourceW(exePath.c_str(), FALSE);
        if (!hUpdate)
        {
            return false;
        }

        for (const auto &[resourceId, data] : resources)
        {
            BOOL success = UpdateResourceW(
                hUpdate,
                RT_RCDATA,
                MAKEINTRESOURCE(resourceId),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                (LPVOID)data.data(),
                static_cast<DWORD>(data.size()));

            if (!success)
            {
                EndUpdateResourceW(hUpdate, TRUE); // Discard
                return false;
            }
        }

        return EndUpdateResourceW(hUpdate, FALSE) != FALSE;
    }

} // namespace autopatch
