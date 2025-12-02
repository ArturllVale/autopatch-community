#include "http.h"
#include <Windows.h>
#include <winhttp.h>
#include <fstream>

#pragma comment(lib, "winhttp.lib")

namespace autopatch
{

    HttpClient::HttpClient()
    {
        m_hSession = WinHttpOpen(
            m_userAgent.c_str(),
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);
    }

    HttpClient::~HttpClient()
    {
        if (m_hSession)
        {
            WinHttpCloseHandle(m_hSession);
        }
    }

    void HttpClient::SetTimeout(int seconds)
    {
        m_timeout = seconds;
        if (m_hSession)
        {
            int timeoutMs = seconds * 1000;
            WinHttpSetTimeouts(m_hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);
        }
    }

    void HttpClient::SetUserAgent(const std::wstring &userAgent)
    {
        m_userAgent = userAgent;
    }

    HttpResponse HttpClient::Get(const std::wstring &url)
    {
        return Get(url, nullptr);
    }

    HttpResponse HttpClient::Get(const std::wstring &url, ProgressCallback progress)
    {
        HttpResponse response;

        if (!m_hSession)
        {
            response.error = L"HTTP session not initialized";
            return response;
        }

        // Parse URL
        URL_COMPONENTS urlComp = {};
        urlComp.dwStructSize = sizeof(urlComp);

        wchar_t hostName[256] = {};
        wchar_t urlPath[2048] = {};
        urlComp.lpszHostName = hostName;
        urlComp.dwHostNameLength = 256;
        urlComp.lpszUrlPath = urlPath;
        urlComp.dwUrlPathLength = 2048;

        if (!WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp))
        {
            response.error = L"Invalid URL";
            return response;
        }

        // Connect
        HINTERNET hConnect = WinHttpConnect(m_hSession, hostName, urlComp.nPort, 0);
        if (!hConnect)
        {
            response.error = L"Connection failed";
            return response;
        }

        // Create request
        DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect, L"GET", urlPath, nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);

        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            response.error = L"Request creation failed";
            return response;
        }

        // Send request
        if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            response.error = L"Send request failed";
            return response;
        }

        // Receive response
        if (!WinHttpReceiveResponse(hRequest, nullptr))
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            response.error = L"Receive response failed";
            return response;
        }

        // Get status code
        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize,
                            WINHTTP_NO_HEADER_INDEX);
        response.statusCode = statusCode;

        // Get content length
        DWORD contentLength = 0;
        DWORD contentLengthSize = sizeof(contentLength);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &contentLength, &contentLengthSize,
                            WINHTTP_NO_HEADER_INDEX);

        // Read data
        std::string body;
        size_t totalRead = 0;
        DWORD bytesAvailable = 0;

        while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0)
        {
            std::vector<char> buffer(bytesAvailable);
            DWORD bytesRead = 0;

            if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead))
            {
                body.append(buffer.data(), bytesRead);
                totalRead += bytesRead;

                if (progress)
                {
                    progress(totalRead, contentLength);
                }
            }
        }

        response.body = std::move(body);
        response.success = (statusCode >= 200 && statusCode < 300);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);

        return response;
    }

    bool HttpClient::DownloadFile(const std::wstring &url, const std::wstring &outputPath,
                                  ProgressCallback progress)
    {
        auto response = Get(url, progress);
        if (!response.success)
        {
            return false;
        }

        std::ofstream file(outputPath, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        file.write(response.body.data(), response.body.size());
        return true;
    }

    HttpResponse HttpClient::Post(const std::wstring &url, const std::string &body,
                                  const std::wstring &contentType)
    {
        HttpResponse response;

        if (!m_hSession)
        {
            response.error = L"HTTP session not initialized";
            return response;
        }

        // Parse URL
        URL_COMPONENTS urlComp = {};
        urlComp.dwStructSize = sizeof(urlComp);

        wchar_t hostName[256] = {};
        wchar_t urlPath[2048] = {};
        urlComp.lpszHostName = hostName;
        urlComp.dwHostNameLength = 256;
        urlComp.lpszUrlPath = urlPath;
        urlComp.dwUrlPathLength = 2048;

        if (!WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp))
        {
            response.error = L"Invalid URL";
            return response;
        }

        // Connect
        HINTERNET hConnect = WinHttpConnect(m_hSession, hostName, urlComp.nPort, 0);
        if (!hConnect)
        {
            response.error = L"Connection failed";
            return response;
        }

        // Create request
        DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect, L"POST", urlPath, nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);

        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            response.error = L"Request creation failed";
            return response;
        }

        // Add content-type header
        std::wstring headers = L"Content-Type: " + contentType;
        WinHttpAddRequestHeaders(hRequest, headers.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);

        // Send request
        if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                (LPVOID)body.data(), static_cast<DWORD>(body.size()),
                                static_cast<DWORD>(body.size()), 0))
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            response.error = L"Send request failed";
            return response;
        }

        // Receive response
        if (!WinHttpReceiveResponse(hRequest, nullptr))
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            response.error = L"Receive response failed";
            return response;
        }

        // Get status code
        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize,
                            WINHTTP_NO_HEADER_INDEX);
        response.statusCode = statusCode;

        // Read data
        std::string responseBody;
        DWORD bytesAvailable = 0;

        while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0)
        {
            std::vector<char> buffer(bytesAvailable);
            DWORD bytesRead = 0;

            if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead))
            {
                responseBody.append(buffer.data(), bytesRead);
            }
        }

        response.body = std::move(responseBody);
        response.success = (statusCode >= 200 && statusCode < 300);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);

        return response;
    }

} // namespace autopatch
