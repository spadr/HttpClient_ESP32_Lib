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

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Connect to NTP Server
    Serial.println("Waiting for NTP time sync...");
    configTime(gmt_offset_sec, daylight_offset_sec, ntp_host);
    while (!time(nullptr))
    {
        delay(1000);
        Serial.print(".");
    }
    getLocalTime(&timeInfo);
    Serial.println("\nNTP time synced");
    Serial.println("Starting HttpClient test...");

    bool useMock = true;
    canaspad::ClientOptions options;
    options.verifySsl = true;
    options.rootCA = isrg_root_x1;
    options.followRedirects = true;
    // options.proxyUrl = "http://160.248.80.91:22";
    canaspad::HttpClient client(options, useMock);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    // リダイレクトレスポンス (302 Found)
    const char *response1 =
        "HTTP/1.1 302 Found\r\n"
        "Location: https://example2.com/redirected\r\n"
        "\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response1, response1 + strlen(response1)));

    // 最終的なレスポンス
    const char *response2 =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 18\r\n\r\n"
        "Redirected page!";
    mockClient->injectResponse(std::vector<uint8_t>(response2, response2 + strlen(response2)));

    // 最初のGETリクエスト
    canaspad::Request request;
    request.setUrl("https://example1.com/init").setMethod(canaspad::HttpMethod::GET);
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
                Serial.println("Sent:");
                Serial.println(requestStr.c_str());
            }
            else if (entry.type == canaspad::CommunicationLog::Entry::Type::Received)
            {
                Serial.println("Received:");
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