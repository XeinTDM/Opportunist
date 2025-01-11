#pragma once

#include <string>
#include <vector>
#include <filesystem>

struct BrowserInfo {
    std::wstring name;
    std::wstring path;
    std::wstring profileName;
    std::wstring bookmarksPath;
    std::wstring historyPath;
    std::wstring passwordsPath;
    std::wstring cookiesPath;
    std::wstring creditCardsPath;
    std::wstring autofillPath;
    std::wstring downloadsPath;
    std::wstring searchQueriesPath;
};

void setBrowserPaths(std::vector<BrowserInfo>& browsers, const std::wstring& localAppData);
