#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <Windows.h>

namespace autopatch
{

    // Classe para manipulação de recursos Win32
    class Resources
    {
    public:
        // Carrega dados de um recurso RCDATA
        static std::vector<uint8_t> LoadRCData(int resourceId, HMODULE hModule = nullptr);

        // Carrega imagem PNG de um recurso
        static std::vector<uint8_t> LoadPNG(int resourceId, HMODULE hModule = nullptr);

        // Carrega string de um recurso
        static std::wstring LoadString(int resourceId, HMODULE hModule = nullptr);

        // Embute dados em um EXE como recurso RCDATA
        static bool EmbedRCData(const std::wstring &exePath, int resourceId,
                                const std::vector<uint8_t> &data);

        // Embute dados em um EXE como recurso RCDATA (de string)
        static bool EmbedRCData(const std::wstring &exePath, int resourceId,
                                const std::string &data);

        // Embute múltiplos recursos de uma vez
        static bool EmbedMultipleResources(const std::wstring &exePath,
                                           const std::vector<std::pair<int, std::vector<uint8_t>>> &resources);

        // Resource IDs padrão
        static constexpr int ID_CONFIG = 1001;
        static constexpr int ID_BACKGROUND = 1002;
        static constexpr int ID_SKIN_DATA = 1003;
        static constexpr int ID_ICON = 1;
    };

} // namespace autopatch
