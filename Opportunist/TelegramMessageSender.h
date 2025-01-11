#pragma once

#include <string>
#include <memory>

class HTTPClient;

class TelegramMessageSender {
    std::wstring botToken;
    std::wstring chatId;
    std::unique_ptr<HTTPClient> httpClient;

public:
    TelegramMessageSender(const std::wstring& token,
        const std::wstring& chat,
        std::unique_ptr<HTTPClient> client);

    bool sendMessage(const std::wstring& message);
    bool sendDocument(const std::wstring& filePath, const std::wstring& caption = L"");
};

