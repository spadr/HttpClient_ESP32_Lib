#include <Arduino.h>
#include <WiFi.h>
#include <HttpClient.h>

using namespace canaspad;

// Replace with your WiFi credentials
#ifndef WIFI_SSID
#define WIFI_SSID "your-ssid"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "your-password"
#endif

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(10);
    }

    Serial.println("\n=== HttpClient_ESP32_Lib Basic Example ===");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    if (!HttpClient::syncTime())
    {
        Serial.println("Warning: time sync failed. HTTPS verification may fail.");
    }

    ClientOptions options;
    options.followRedirects = true;
    options.verifySsl = true;
    HttpClient client(options);
    client.enableCookies(true);

    Request request;
    request.setUrl("https://httpbin.org/post")
        .setMethod(HttpMethod::POST)
        .addHeader("Content-Type", "application/json")
        .setBody("{\"message\":\"Hello from ESP32!\"}");

    auto result = client.send(request);
    if (result.isSuccess())
    {
        Serial.printf("Status: %d\n", result.value().statusCode);
        Serial.println(result.value().body.c_str());
    }
    else
    {
        Serial.printf("Error: %s (code=%d)\n",
                      result.error().message.c_str(),
                      static_cast<int>(result.error().code));
    }
}

void loop()
{
    delay(10000);
}
