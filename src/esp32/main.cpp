#include <Arduino.h>
#include <WiFi.h>
#include "../Config.h" // ConfigExample.h ではなく Config.h を使用

#include "../HttpClient.h"
#include "../core/Request.h"

using namespace canaspad;

void setup()
{
    delay(2000); // シリアルモニタ接続待ち

    Serial.begin(115200);
    Serial.println("\n=== HttpClient ESP32 Library - Main Execution ===");

    // WiFi接続
    Serial.println("Connecting to WiFi...");
    WiFi.begin(Config::ssid, Config::password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // HTTP時刻同期 (自前サーバー)
    if (HttpClient::syncTime("https://timestamp.canaspad.net/"))
    {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            Serial.println("Time synchronized:");
            Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        }
    }
    else
    {
        Serial.println("Warning: Time sync failed. SSL verification might fail.");
    }

    // NTP同期を使用する場合
    /*
    configTime(9 * 3600, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");
    Serial.println("Waiting for NTP time sync...");
    struct tm timeinfo;

    // 時刻が設定されるまで待機（NTP同期はバックグラウンドで行われるためポーリングが必要）
    while (!getLocalTime(&timeinfo))
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nTime synchronized via NTP:");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    */

    // HTTPS POSTリクエストのデモ実行
    Serial.println("\n--- Sending Demo POST Request ---");

    ClientOptions options;
    options.followRedirects = true;
    options.rootCA = Config::gts_root_r4; // Google Trust Services Root R4
    HttpClient client(options, false);

    std::string postData = "{\"test\":\"data\",\"timestamp\":\"" +
                           std::to_string(millis()) + "\"}";

    Request request;
    request.setUrl("https://demo1.canaspad.net/post")
        .setMethod(canaspad::HttpMethod::POST)
        .addHeader("Content-Type", "application/json")
        .setBody(postData);

    Serial.println("Target: https://demo1.canaspad.net/post");
    auto result = client.send(request);

    if (result.isSuccess())
    {
        auto response = result.value();
        Serial.printf("Status Code: %d\n", response.statusCode);
        Serial.printf("Response Body: %s\n", response.body.c_str());
    }
    else
    {
        Serial.printf("Error: %s (Code: %d)\n", result.error().message.c_str(), (int)result.error().code);
    }

    Serial.println("\n--- Demo Finished ---");
}

void loop()
{
    delay(10000);
    Serial.println("Looping...");
}
