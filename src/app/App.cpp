#include "App.hpp"

#ifndef ARDUINO_ARCH_NATIVE
#include <Arduino.h>
#include <WiFi.h>
#else
#include "../native_arduino_compat.h"
#endif

#include "../HttpClient.h"
#include "../core/mock/MockWiFiClientSecure.h"

#include "../ConfigExample.h"

namespace app {

void init() {
    Serial.begin(115200);
    Serial.println("HttpClient ESP32 Library - Application Starting...");

#ifndef ARDUINO_ARCH_NATIVE
    // WiFi接続
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ConfigExample::ssid, ConfigExample::password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");

    // NTP時刻同期
    Serial.println("Waiting for NTP time sync...");
    configTime(ConfigExample::gmt_offset_sec, ConfigExample::daylight_offset_sec, ConfigExample::ntp_host);
    struct tm timeInfo;
    while (!time(nullptr)) {
        delay(1000);
        Serial.print(".");
    }
    getLocalTime(&timeInfo);
    Serial.println("\nNTP time synced");
    
    // 時刻表示
    delay(1000);
    time_t now_sync;
    struct tm timeinfo_sync;
    time(&now_sync);
    localtime_r(&now_sync, &timeinfo_sync);
    Serial.print("Current time after sync: ");
    char time_buf[30];
    snprintf(time_buf, sizeof(time_buf), "%04d/%02d/%02d %02d:%02d:%02d",
            (timeinfo_sync.tm_year) + 1900, (timeinfo_sync.tm_mon) + 1, timeinfo_sync.tm_mday,
            timeinfo_sync.tm_hour, timeinfo_sync.tm_min, timeinfo_sync.tm_sec);
    Serial.println(time_buf);
#else
    Serial.println("Native environment - WiFi and NTP simulation");
    delay(2000); // シミュレート
#endif

    Serial.println("Application initialization completed");
}

void runDemo() {
    Serial.println("Starting HttpClient demo...");

    // デモ設定
    bool useMock = false;
    canaspad::ClientOptions options;
    options.verifySsl = true;
#ifndef ARDUINO_ARCH_NATIVE
    options.rootCA = ConfigExample::isrg_root_x1;
#endif
    options.followRedirects = true;
    
    canaspad::HttpClient client(options, useMock);

#ifndef ARDUINO_ARCH_NATIVE
    // モックレスポンス設定（ESP32環境でのテスト用）
    if (useMock) {
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

        // HTTPS テスト
        Serial.println("Starting HTTPS GET test with mock...");
        canaspad::Request request;
        request.setUrl("https://example1.com/init").setMethod(canaspad::HttpMethod::GET);
        auto result = client.send(request);

        if (result.isSuccess()) {
            const auto &httpResult = result.value();
            Serial.printf("HTTPS Status code: %d\n", httpResult.statusCode);
            Serial.printf("HTTPS Body: %s\n", httpResult.body.c_str());
        } else {
            const auto &error = result.error();
            Serial.printf("HTTPS Error: %d, %s\n", static_cast<int>(error.code), error.message.c_str());
        }
    }

    // HTTP GET テスト (実機)
    Serial.println("Starting HTTP GET test to 192.168.137.1...");
    canaspad::Request requestHttp;
    requestHttp.setUrl("http://192.168.137.1/")
        .setMethod(canaspad::HttpMethod::GET);

    auto resultHttp = client.send(requestHttp);

    if (resultHttp.isSuccess()) {
        const auto &httpResult = resultHttp.value();
        Serial.printf("HTTP Status code: %d\n", httpResult.statusCode);
        Serial.printf("HTTP Body: %s\n", httpResult.body.c_str());
    } else {
        const auto &error = resultHttp.error();
        Serial.printf("HTTP Error: %d, %s\n", static_cast<int>(error.code), error.message.c_str());
    }
#else
    // Native環境での簡易テスト
    Serial.println("Native environment - HTTP client simulation");
    canaspad::Request request;
    request.setUrl("http://example.com/").setMethod(canaspad::HttpMethod::GET);
    
    auto result = client.send(request);
    if (result.isSuccess()) {
        Serial.println("Native HTTP test - Success simulation");
    } else {
        Serial.println("Native HTTP test - Error simulation");  
    }
#endif

    Serial.println("HttpClient demo completed");
}

void tick() {
    // 現在は特に継続処理なし
    delay(1000);
}

} // namespace app