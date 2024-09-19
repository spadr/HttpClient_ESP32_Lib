#ifndef PIO_UNIT_TESTING
#include <Arduino.h>
#include <WiFi.h>
#include "HttpClient.h"

#include "../src/HttpClient.h"
#include "../src/core/mock/MockWiFiClientSecure.h"

// Config.h が存在するかどうかをチェック
#ifdef CONFIG_H_EXISTS
#include "Config.h"
using namespace Config; // Config 名前空間を使う
#else
#include "ConfigExample.h"
using namespace ConfigExample; // ConfigExample 名前空間を使う
#endif

struct tm timeInfo;

void setup()
{
    Serial.begin(115200);
    bool useMock = true;
    canaspad::ClientOptions options;
    options.proxyUrl = "http://proxy.example.com:8080";
    canaspad::HttpClient client(options, useMock);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    // プロキシ経由での通信のレスポンスを設定
    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nHello";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("http://example.com/test").setMethod(canaspad::HttpMethod::GET);

    auto result = client.send(request);

    if (result.isSuccess())
    {
        const auto &httpResult = result.value();
        Serial.printf("Status code: %d\n", httpResult.statusCode);
        Serial.printf("Body: %s\n", httpResult.body.c_str());
    }
    else
    {
        const auto &error = result.error();
        Serial.printf("Error: %d, %s\n", static_cast<int>(error.code), error.message.c_str());
    }
    if (useMock)
    {
        Serial.println("--------------------");
        const auto &log = mockClient->getCommunicationLog();
        for (const auto &entry : log.getLog())
        {
            // 送信ログと受信ログを表示
            if (entry.type == canaspad::CommunicationLog::Entry::Type::Sent)
            {
                std::string requestStr(entry.data.begin(), entry.data.end());
                Serial.println(requestStr.c_str());
            }
            else if (entry.type == canaspad::CommunicationLog::Entry::Type::Received)
            {
                std::string responseStr(entry.data.begin(), entry.data.end());
                Serial.println(responseStr.c_str());
            }
        }
        Serial.println("--------------------");
    }
}

void loop()
{
    // ここでは何もしない
    delay(1000);
}

#endif // PIO_UNIT_TESTING