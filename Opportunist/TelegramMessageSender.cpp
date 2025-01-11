#include "TelegramMessageSender.h"

#include <winsock2.h>
#include <fstream>
#include <vector>
#include "Utilities.h"

TelegramMessageSender::TelegramMessageSender(const std::wstring& token,
    const std::wstring& chat,
    std::unique_ptr<HTTPClient> client)
    : botToken(token), chatId(chat), httpClient(std::move(client)) {
}

bool TelegramMessageSender::sendMessage(const std::wstring& message) {
    std::wstring host = L"api.telegram.org";
    std::wstring path = L"/bot" + botToken + L"/sendMessage";
    std::wstring headers = L"Content-Type: application/x-www-form-urlencoded";

    std::string data = "chat_id=" + urlEncode(chatId) + "&text=" + urlEncode(message);
    auto resp = httpClient->post(host, path, headers, data);
    return resp.success;
}

bool TelegramMessageSender::sendDocument(const std::wstring& filePath, const std::wstring& caption) {
    std::wstring host = L"api.telegram.org";
    std::wstring path = L"/bot" + botToken + L"/sendDocument";
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::wstring headers = L"Content-Type: multipart/form-data; boundary=" + stringToWString(boundary);

    std::ifstream fileStream(filePath, std::ios::binary);
    if (!fileStream) return false;
    std::vector<char> fileContent((std::istreambuf_iterator<char>(fileStream)),
        std::istreambuf_iterator<char>());
    fileStream.close();
    if (fileContent.empty()) return false;

    std::string body;
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n";
    body += wstringToString(chatId) + "\r\n";

    if (!caption.empty()) {
        body += "--" + boundary + "\r\n";
        body += "Content-Disposition: form-data; name=\"caption\"\r\n\r\n";
        body += wstringToString(caption) + "\r\n";
    }

    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"document\"; filename=\"data.zip\"\r\n";
    body += "Content-Type: application/zip\r\n\r\n";
    body += std::string(fileContent.begin(), fileContent.end()) + "\r\n";
    body += "--" + boundary + "--\r\n";

    auto resp = httpClient->post(host, path, headers, body);
    return resp.success;
}
