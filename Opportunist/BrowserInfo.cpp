#include "BrowserInfo.h"

#include <windows.h>
#include <unordered_map>
#include <filesystem>
#include <shlobj.h>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

bool isProfileDirectory(const fs::path& dirName) {
    std::wstring name = dirName.filename().wstring();
    return name == L"Default" ||
        (name.find(L"Profile ") == 0 && name.size() > 8) ||
        name.find(L"Guest Profile") == 0 ||
        name.find(L"Test Profile") == 0;
}

void setBrowserPaths(std::vector<BrowserInfo>& browsers, const std::wstring& localAppData) {
    std::unordered_map<std::wstring, std::wstring> browserPaths = {
        {L"Chromium", L"Chromium\\User Data"},
        {L"GoogleChrome", L"Google\\Chrome\\User Data"},
        {L"GoogleChromeSxS", L"Google\\Chrome SxS\\User Data"},
        {L"GoogleChromeBeta", L"Google\\Chrome Beta\\User Data"},
        {L"GoogleChromeDev", L"Google\\Chrome Dev\\User Data"},
        {L"GoogleChromeUnstable", L"Google\\Chrome Unstable\\User Data"},
        {L"GoogleChromeCanary", L"Google\\Chrome Canary\\User Data"},
        {L"Edge", L"Microsoft\\Edge\\User Data"},
        {L"Brave", L"BraveSoftware\\Brave-Browser\\User Data"},
        {L"OperaGX", L"Opera Software\\Opera GX Stable"},
        {L"Opera", L"Opera Software\\Opera Stable"},
        {L"OperaNeon", L"Opera Software\\Opera Neon\\User Data"},
        {L"Vivaldi", L"Vivaldi\\User Data"},
        {L"Blisk", L"Blisk\\User Data"},
        {L"Epic", L"Epic Privacy Browser\\User Data"},
        {L"SRWareIron", L"SRWare Iron\\User Data"},
        {L"ComodoDragon", L"Comodo\\Dragon\\User Data"},
        {L"Yandex", L"Yandex\\YandexBrowser\\User Data"},
        {L"YandexCanary", L"Yandex\\YandexBrowserCanary\\User Data"},
        {L"YandexDeveloper", L"Yandex\\YandexBrowserDeveloper\\User Data"},
        {L"YandexBeta", L"Yandex\\YandexBrowserBeta\\User Data"},
        {L"YandexTech", L"Yandex\\YandexBrowserTech\\User Data"},
        {L"YandexSxS", L"Yandex\\YandexBrowserSxS\\User Data"},
        {L"Slimjet", L"Slimjet\\User Data"},
        {L"UC", L"UCBrowser\\User Data"},
        {L"Avast", L"AVAST Software\\Browser\\User Data"},
        {L"CentBrowser", L"CentBrowser\\User Data"},
        {L"Kinza", L"Kinza\\User Data"},
        {L"Chedot", L"Chedot\\User Data"},
        {L"360Browser", L"360Browser\\User Data"},
        {L"Falkon", L"Falkon\\User Data"},
        {L"AVG", L"AVG\\Browser\\User Data"},
        {L"CocCoc", L"CocCoc\\Browser\\User Data"},
        {L"Torch", L"Torch\\User Data"},
        {L"NaverWhale", L"Naver\\Whale\\User Data"},
        {L"Maxthon", L"Maxthon\\User Data"},
        {L"Iridium", L"Iridium\\User Data"},
        {L"Puffin", L"CloudMosa\\Puffin\\User Data"},
        {L"Amigo", L"Amigo\\User Data"}
    };

    std::vector<BrowserInfo> updatedBrowsers;

    for (const auto& browser : browsers) {
        auto it = browserPaths.find(browser.name);
        if (it != browserPaths.end()) {
            fs::path browserBasePath = fs::path(localAppData) / it->second;

            if (fs::exists(browserBasePath) && fs::is_directory(browserBasePath)) {
                for (const auto& entry : fs::directory_iterator(browserBasePath)) {
                    if (entry.is_directory() && isProfileDirectory(entry.path())) {
                        BrowserInfo profileInfo = browser;
                        profileInfo.path = it->second;
                        profileInfo.profileName = entry.path().filename().wstring();

                        fs::path profileBasePath = entry.path();

                        profileInfo.bookmarksPath = (profileBasePath / L"Bookmarks").wstring();
                        profileInfo.historyPath = (profileBasePath / L"History").wstring();
                        profileInfo.passwordsPath = (profileBasePath / L"Login Data").wstring();
                        profileInfo.creditCardsPath = (profileBasePath / L"Web Data").wstring();
                        profileInfo.autofillPath = (profileBasePath / L"Autofill").wstring();
                        profileInfo.searchQueriesPath = profileInfo.historyPath;
                        profileInfo.downloadsPath = profileInfo.historyPath;

                        fs::path cookiesPathNew = profileBasePath / L"Network" / L"Cookies";
                        fs::path cookiesPathOld = profileBasePath / L"Cookies";

                        if (fs::exists(cookiesPathNew)) {
                            profileInfo.cookiesPath = cookiesPathNew.wstring();
                        }
                        else if (fs::exists(cookiesPathOld)) {
                            profileInfo.cookiesPath = cookiesPathOld.wstring();
                        }
                        else {
                            profileInfo.cookiesPath = L"";
                        }

                        updatedBrowsers.push_back(profileInfo);
                    }
                }

                if (updatedBrowsers.empty()) {
                    BrowserInfo defaultProfile = browser;
                    defaultProfile.profileName = L"Default";

                    fs::path defaultBasePath = browserBasePath / L"Default";

                    defaultProfile.bookmarksPath = (defaultBasePath / L"Bookmarks").wstring();
                    defaultProfile.historyPath = (defaultBasePath / L"History").wstring();
                    defaultProfile.passwordsPath = (defaultBasePath / L"Login Data").wstring();
                    defaultProfile.creditCardsPath = (defaultBasePath / L"Web Data").wstring();
                    defaultProfile.autofillPath = (defaultBasePath / L"Autofill").wstring();
                    defaultProfile.searchQueriesPath = defaultProfile.historyPath;
                    defaultProfile.downloadsPath = defaultProfile.historyPath;

                    fs::path cookiesPathNew = defaultBasePath / L"Network" / L"Cookies";
                    fs::path cookiesPathOld = defaultBasePath / L"Cookies";

                    if (fs::exists(cookiesPathNew)) {
                        defaultProfile.cookiesPath = cookiesPathNew.wstring();
                    }
                    else if (fs::exists(cookiesPathOld)) {
                        defaultProfile.cookiesPath = cookiesPathOld.wstring();
                    }
                    else {
                        defaultProfile.cookiesPath = L"";
                    }

                    updatedBrowsers.push_back(defaultProfile);
                }
            }
        }
        else {
            BrowserInfo emptyBrowser = browser;
            emptyBrowser.bookmarksPath = L"";
            emptyBrowser.historyPath = L"";
            emptyBrowser.passwordsPath = L"";
            emptyBrowser.cookiesPath = L"";
            emptyBrowser.creditCardsPath = L"";
            emptyBrowser.autofillPath = L"";
            emptyBrowser.profileName = L"";
            updatedBrowsers.push_back(emptyBrowser);
        }
    }

    browsers = std::move(updatedBrowsers);
}