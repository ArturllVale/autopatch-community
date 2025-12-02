#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace autopatch
{

    // Callback de progresso: (bytesReceived, totalBytes)
    using ProgressCallback = std::function<void(size_t, size_t)>;

    // Resultado de uma requisição HTTP
    struct HttpResponse
    {
        int statusCode = 0;
        std::string body;
        std::wstring error;
        bool success = false;
    };

    // Cliente HTTP usando WinHTTP
    class HttpClient
    {
    public:
        HttpClient();
        ~HttpClient();

        // GET request
        HttpResponse Get(const std::wstring &url);

        // GET com callback de progresso
        HttpResponse Get(const std::wstring &url, ProgressCallback progress);

        // Download para arquivo
        bool DownloadFile(const std::wstring &url, const std::wstring &outputPath,
                          ProgressCallback progress = nullptr);

        // POST request
        HttpResponse Post(const std::wstring &url, const std::string &body,
                          const std::wstring &contentType = L"application/json");

        // Configurações
        void SetTimeout(int seconds);
        void SetUserAgent(const std::wstring &userAgent);

    private:
        void *m_hSession = nullptr;
        int m_timeout = 30;
        std::wstring m_userAgent = L"AutoPatcher/1.0";
    };

} // namespace autopatch
