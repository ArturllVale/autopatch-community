// AutoPatch Embedder CLI
// Embeds configuration and resources into the patcher executable
//
// Usage:
//   embedder.exe --config <config.json> --template <template.exe> --output <output.exe> [options]
//
// Options:
//   --config <path>       Path to JSON configuration file (required)
//   --template <path>     Path to template EXE (AutoPatcher.exe) (required)
//   --output <path>       Path for output EXE (required)
//   --background <path>   Path to background image (optional)
//   --icon <path>         Path to icon file (optional)
//   --help                Show this help message
//
// Exit codes:
//   0 - Success
//   1 - Invalid arguments
//   2 - File not found
//   3 - Copy failed
//   4 - Resource embedding failed
//   5 - Invalid JSON

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

// Resource IDs (must match the patcher's expectations)
constexpr WORD ID_CONFIG = 1001;
constexpr WORD ID_SKIN = 1002;
constexpr WORD ID_BACKGROUND = 1003;
constexpr WORD ID_HTML_CONTENT = 1008;
constexpr WORD ID_CSS_CONTENT = 1009;
constexpr WORD ID_JS_CONTENT = 1010;

// Icon structures for ICO file parsing
#pragma pack(push, 1)
struct ICONDIR
{
    WORD idReserved; // Reserved (must be 0)
    WORD idType;     // Resource Type (1 for icons)
    WORD idCount;    // Number of images
};

struct ICONDIRENTRY
{
    BYTE bWidth;         // Width, in pixels
    BYTE bHeight;        // Height, in pixels
    BYTE bColorCount;    // Number of colors (0 if >= 8bpp)
    BYTE bReserved;      // Reserved (must be 0)
    WORD wPlanes;        // Color Planes
    WORD wBitCount;      // Bits per pixel
    DWORD dwBytesInRes;  // Size of image data
    DWORD dwImageOffset; // Offset to image data in file
};

struct GRPICONDIRENTRY
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    WORD nID; // ID of the icon (replaces offset)
};

struct GRPICONDIR
{
    WORD idReserved;
    WORD idType;
    WORD idCount;
    // GRPICONDIRENTRY entries follow
};
#pragma pack(pop)

// Convert string to wide string
std::wstring ToWide(const std::string &str)
{
    if (str.empty())
        return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size);
    return result;
}

// Convert wide string to string
std::string ToNarrow(const std::wstring &wstr)
{
    if (wstr.empty())
        return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], size, nullptr, nullptr);
    return result;
}

// Read file into byte vector
std::vector<uint8_t> ReadFile(const std::wstring &path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
    {
        return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char *>(buffer.data()), size))
    {
        return {};
    }

    return buffer;
}

// Read text file
std::string ReadTextFile(const std::wstring &path)
{
    std::ifstream file(path);
    if (!file)
    {
        return "";
    }

    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// Update resource in executable
bool EmbedResource(HANDLE hUpdate, WORD resourceId, const void *data, DWORD size)
{
    return ::UpdateResourceW(
               hUpdate,
               MAKEINTRESOURCEW(10), // RT_RCDATA = 10
               MAKEINTRESOURCEW(resourceId),
               MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
               const_cast<void *>(data),
               size) != FALSE;
}

// Embed icon file into executable
// Returns true on success
bool EmbedIcon(HANDLE hUpdate, const std::wstring &iconPath)
{
    // Read the icon file
    std::vector<uint8_t> iconData = ReadFile(iconPath);
    if (iconData.size() < sizeof(ICONDIR))
    {
        std::cerr << "Error: Icon file too small" << std::endl;
        return false;
    }

    // Parse ICONDIR header
    const ICONDIR *pIconDir = reinterpret_cast<const ICONDIR *>(iconData.data());

    if (pIconDir->idReserved != 0 || pIconDir->idType != 1)
    {
        std::cerr << "Error: Invalid icon file format" << std::endl;
        return false;
    }

    WORD iconCount = pIconDir->idCount;
    if (iconCount == 0)
    {
        std::cerr << "Error: No icons in file" << std::endl;
        return false;
    }

    std::cout << "  - Icon contains " << iconCount << " image(s)" << std::endl;

    // Verify file size
    size_t expectedMinSize = sizeof(ICONDIR) + iconCount * sizeof(ICONDIRENTRY);
    if (iconData.size() < expectedMinSize)
    {
        std::cerr << "Error: Icon file is truncated" << std::endl;
        return false;
    }

    // Get pointer to icon entries
    const ICONDIRENTRY *pEntries = reinterpret_cast<const ICONDIRENTRY *>(
        iconData.data() + sizeof(ICONDIR));

    // Create GRPICONDIR structure
    size_t grpSize = sizeof(GRPICONDIR) + iconCount * sizeof(GRPICONDIRENTRY);
    std::vector<uint8_t> grpData(grpSize);

    GRPICONDIR *pGrpDir = reinterpret_cast<GRPICONDIR *>(grpData.data());
    pGrpDir->idReserved = 0;
    pGrpDir->idType = 1;
    pGrpDir->idCount = iconCount;

    GRPICONDIRENTRY *pGrpEntries = reinterpret_cast<GRPICONDIRENTRY *>(
        grpData.data() + sizeof(GRPICONDIR));

    // First, remove existing icon resources (ID 1)
    // We'll use icon IDs starting from 1
    const WORD baseIconId = 1;

    // Embed each individual icon image as RT_ICON
    for (WORD i = 0; i < iconCount; i++)
    {
        const ICONDIRENTRY &entry = pEntries[i];
        WORD iconId = baseIconId + i;

        // Verify offset and size
        if (entry.dwImageOffset + entry.dwBytesInRes > iconData.size())
        {
            std::cerr << "Error: Icon image data extends beyond file" << std::endl;
            return false;
        }

        // Get the icon image data
        const void *imageData = iconData.data() + entry.dwImageOffset;

        // Embed as RT_ICON (RT_ICON = 3)
        if (!UpdateResourceW(hUpdate, MAKEINTRESOURCEW(3), MAKEINTRESOURCEW(iconId),
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                             const_cast<void *>(imageData),
                             entry.dwBytesInRes))
        {
            std::cerr << "Error: Failed to embed RT_ICON " << iconId
                      << " (Error: " << GetLastError() << ")" << std::endl;
            return false;
        }

        // Fill in the group entry
        pGrpEntries[i].bWidth = entry.bWidth;
        pGrpEntries[i].bHeight = entry.bHeight;
        pGrpEntries[i].bColorCount = entry.bColorCount;
        pGrpEntries[i].bReserved = entry.bReserved;
        pGrpEntries[i].wPlanes = entry.wPlanes;
        pGrpEntries[i].wBitCount = entry.wBitCount;
        pGrpEntries[i].dwBytesInRes = entry.dwBytesInRes;
        pGrpEntries[i].nID = iconId;

        std::cout << "    - Image " << (i + 1) << ": "
                  << (entry.bWidth == 0 ? 256 : (int)entry.bWidth) << "x"
                  << (entry.bHeight == 0 ? 256 : (int)entry.bHeight)
                  << " (" << entry.dwBytesInRes << " bytes)" << std::endl;
    }

    // Embed RT_GROUP_ICON (ID 1 is typically the main application icon, RT_GROUP_ICON = 14)
    if (!UpdateResourceW(hUpdate, MAKEINTRESOURCEW(14), MAKEINTRESOURCEW(1),
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                         grpData.data(),
                         static_cast<DWORD>(grpData.size())))
    {
        std::cerr << "Error: Failed to embed RT_GROUP_ICON (Error: " << GetLastError() << ")" << std::endl;
        return false;
    }

    return true;
}

// Print usage
void PrintUsage()
{
    std::cout << R"(
AutoPatch Embedder CLI v1.0

Usage:
  embedder.exe --config <config.json> --template <template.exe> --output <output.exe> [options]

Options:
  --config <path>       Path to JSON configuration file (required)
  --template <path>     Path to template EXE (AutoPatcher.exe) (required)
  --output <path>       Path for output EXE (required)
  --background <path>   Path to background image (optional)
  --icon <path>         Path to icon file (optional)
  --help                Show this help message

Exit codes:
  0 - Success
  1 - Invalid arguments
  2 - File not found
  3 - Copy failed
  4 - Resource embedding failed
  5 - Invalid JSON

Example:
  embedder.exe --config patcher.json --template AutoPatcher.exe --output MyPatcher.exe --background bg.png
)";
}

// Parse command line arguments
struct Arguments
{
    std::wstring configPath;
    std::wstring templatePath;
    std::wstring outputPath;
    std::wstring backgroundPath;
    std::wstring iconPath;
    bool showHelp = false;
};

Arguments ParseArgs(int argc, wchar_t *argv[])
{
    Arguments args;

    for (int i = 1; i < argc; i++)
    {
        std::wstring arg = argv[i];

        if (arg == L"--help" || arg == L"-h")
        {
            args.showHelp = true;
        }
        else if (arg == L"--config" && i + 1 < argc)
        {
            args.configPath = argv[++i];
        }
        else if (arg == L"--template" && i + 1 < argc)
        {
            args.templatePath = argv[++i];
        }
        else if (arg == L"--output" && i + 1 < argc)
        {
            args.outputPath = argv[++i];
        }
        else if (arg == L"--background" && i + 1 < argc)
        {
            args.backgroundPath = argv[++i];
        }
        else if (arg == L"--icon" && i + 1 < argc)
        {
            args.iconPath = argv[++i];
        }
    }

    return args;
}

int wmain(int argc, wchar_t *argv[])
{
    // Parse arguments
    Arguments args = ParseArgs(argc, argv);

    if (args.showHelp)
    {
        PrintUsage();
        return 0;
    }

    // Validate required arguments
    if (args.configPath.empty())
    {
        std::cerr << "Error: --config is required" << std::endl;
        PrintUsage();
        return 1;
    }

    if (args.templatePath.empty())
    {
        std::cerr << "Error: --template is required" << std::endl;
        PrintUsage();
        return 1;
    }

    if (args.outputPath.empty())
    {
        std::cerr << "Error: --output is required" << std::endl;
        PrintUsage();
        return 1;
    }

    // Check if files exist
    if (!fs::exists(args.configPath))
    {
        std::cerr << "Error: Config file not found: " << ToNarrow(args.configPath) << std::endl;
        return 2;
    }

    if (!fs::exists(args.templatePath))
    {
        std::cerr << "Error: Template file not found: " << ToNarrow(args.templatePath) << std::endl;
        return 2;
    }

    if (!args.backgroundPath.empty() && !fs::exists(args.backgroundPath))
    {
        std::cerr << "Error: Background file not found: " << ToNarrow(args.backgroundPath) << std::endl;
        return 2;
    }

    // Read config file
    std::string configContent = ReadTextFile(args.configPath);
    if (configContent.empty())
    {
        std::cerr << "Error: Failed to read config file" << std::endl;
        return 2;
    }

    // Validate JSON
    try
    {
        json j = json::parse(configContent);
        std::cout << "Config loaded: " << j.value("serverName", "Unknown") << std::endl;
    }
    catch (const json::exception &e)
    {
        std::cerr << "Error: Invalid JSON - " << e.what() << std::endl;
        return 5;
    }

    // Copy template to output
    std::cout << "Copying template..." << std::endl;
    try
    {
        fs::copy_file(args.templatePath, args.outputPath, fs::copy_options::overwrite_existing);
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "Error: Failed to copy template - " << e.what() << std::endl;
        return 3;
    }

    // Begin updating resources
    std::cout << "Embedding resources..." << std::endl;
    HANDLE hUpdate = BeginUpdateResourceW(args.outputPath.c_str(), FALSE);
    if (!hUpdate)
    {
        std::cerr << "Error: Failed to open output file for resource update (Error: " << GetLastError() << ")" << std::endl;
        return 4;
    }

    bool success = true;

    // Embed config
    std::cout << "  - Embedding configuration (ID: " << ID_CONFIG << ")..." << std::endl;
    if (!EmbedResource(hUpdate, ID_CONFIG, configContent.data(), static_cast<DWORD>(configContent.size())))
    {
        std::cerr << "Error: Failed to embed config (Error: " << GetLastError() << ")" << std::endl;
        success = false;
    }

    // Embed background image if provided
    if (success && !args.backgroundPath.empty())
    {
        std::cout << "  - Embedding background image (ID: " << ID_BACKGROUND << ")..." << std::endl;
        std::vector<uint8_t> bgData = ReadFile(args.backgroundPath);
        if (bgData.empty())
        {
            std::cerr << "Error: Failed to read background image" << std::endl;
            success = false;
        }
        else if (!EmbedResource(hUpdate, ID_BACKGROUND, bgData.data(), static_cast<DWORD>(bgData.size())))
        {
            std::cerr << "Error: Failed to embed background (Error: " << GetLastError() << ")" << std::endl;
            success = false;
        }
        else
        {
            std::cout << "    Background size: " << bgData.size() << " bytes" << std::endl;
        }
    }

    // Embed HTML/CSS/JS content for HTML mode
    try
    {
        json j = json::parse(configContent);
        std::string uiMode = j.value("uiMode", "image");

        if (uiMode == "html")
        {
            std::cout << "  - HTML mode detected, embedding web content..." << std::endl;

            // Extract and embed HTML content
            if (j.contains("htmlContent") && j["htmlContent"].is_string())
            {
                std::string htmlContent = j["htmlContent"].get<std::string>();
                if (!htmlContent.empty())
                {
                    std::cout << "  - Embedding HTML content (ID: " << ID_HTML_CONTENT << ")..." << std::endl;
                    if (!EmbedResource(hUpdate, ID_HTML_CONTENT, htmlContent.data(), static_cast<DWORD>(htmlContent.size())))
                    {
                        std::cerr << "Warning: Failed to embed HTML content" << std::endl;
                    }
                    else
                    {
                        std::cout << "    HTML size: " << htmlContent.size() << " bytes" << std::endl;
                    }
                }
            }

            // Extract and embed CSS content
            if (j.contains("cssContent") && j["cssContent"].is_string())
            {
                std::string cssContent = j["cssContent"].get<std::string>();
                if (!cssContent.empty())
                {
                    std::cout << "  - Embedding CSS content (ID: " << ID_CSS_CONTENT << ")..." << std::endl;
                    if (!EmbedResource(hUpdate, ID_CSS_CONTENT, cssContent.data(), static_cast<DWORD>(cssContent.size())))
                    {
                        std::cerr << "Warning: Failed to embed CSS content" << std::endl;
                    }
                    else
                    {
                        std::cout << "    CSS size: " << cssContent.size() << " bytes" << std::endl;
                    }
                }
            }

            // Extract and embed JS content
            if (j.contains("jsContent") && j["jsContent"].is_string())
            {
                std::string jsContent = j["jsContent"].get<std::string>();
                if (!jsContent.empty())
                {
                    std::cout << "  - Embedding JS content (ID: " << ID_JS_CONTENT << ")..." << std::endl;
                    if (!EmbedResource(hUpdate, ID_JS_CONTENT, jsContent.data(), static_cast<DWORD>(jsContent.size())))
                    {
                        std::cerr << "Warning: Failed to embed JS content" << std::endl;
                    }
                    else
                    {
                        std::cout << "    JS size: " << jsContent.size() << " bytes" << std::endl;
                    }
                }
            }
        }
    }
    catch (const json::exception &e)
    {
        std::cerr << "Warning: Could not parse config for HTML content - " << e.what() << std::endl;
    }

    // Embed icon if provided
    if (success && !args.iconPath.empty())
    {
        if (!fs::exists(args.iconPath))
        {
            std::cerr << "Warning: Icon file not found: " << ToNarrow(args.iconPath) << std::endl;
        }
        else
        {
            std::cout << "  - Embedding icon (RT_GROUP_ICON + RT_ICON)..." << std::endl;
            if (!EmbedIcon(hUpdate, args.iconPath))
            {
                std::cerr << "Warning: Failed to embed icon, continuing without icon" << std::endl;
                // Don't fail the whole process for icon embedding failure
            }
            else
            {
                std::cout << "    Icon embedded successfully" << std::endl;
            }
        }
    }

    // Finalize resource update
    if (!EndUpdateResourceW(hUpdate, !success))
    {
        std::cerr << "Error: Failed to finalize resource update (Error: " << GetLastError() << ")" << std::endl;
        return 4;
    }

    if (success)
    {
        std::cout << "Success! Patcher created: " << ToNarrow(args.outputPath) << std::endl;

        // Show file size
        auto fileSize = fs::file_size(args.outputPath);
        std::cout << "Output size: " << fileSize << " bytes (" << (fileSize / 1024) << " KB)" << std::endl;

        return 0;
    }

    return 4;
}
