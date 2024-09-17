#ifndef PIO_UNIT_TESTING
#include <Arduino.h>
#include <WiFi.h>
#include "HttpClient.h"

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

    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options);

    // タイムアウトを設定
    canaspad::HttpClient::Timeouts timeouts;
    timeouts.read = std::chrono::seconds(5); // 読み取りタイムアウトを5秒に設定
    client.setTimeouts(timeouts);

    // テスト1: 基本的なGETリクエスト
    Serial.println("Test 1: Basic GET request");

    canaspad::Request request;
    request.setUrl("http://example.com").setMethod(canaspad::Request::Method::GET);

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
}

void loop()
{
    // ここでは何もしない
    delay(1000);
}

#endif // PIO_UNIT_TESTING