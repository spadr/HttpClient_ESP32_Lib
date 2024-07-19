#include <Arduino.h>
#include <WiFi.h>
#include "HttpClient.h"
#include "Config.h"

struct tm timeInfo;

#ifndef PIO_UNIT_TESTING

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

    // HttpClient setup
    canaspad::ClientOptions options;
    options.followRedirects = true;
    options.verifySsl = true;
    options.rootCA = isrg_root_x1;
    options.clientCert = client_cert;
    options.clientPrivateKey = client_key;

    Serial.println("Creating HttpClient");

    canaspad::HttpClient client(options);

    Serial.println("HttpClient created");

    Serial.println("CA certificate set");

    // Request setup
    canaspad::Request request;

    Serial.println("Creating request");
    request.setUrl(api_url)
        .setMethod(canaspad::Request::Method::GET);

    Serial.println("Request created");

    // Send request
    auto result = client.send(request);
    Serial.println("Response received");

    if (result.isSuccess())
    {
        const auto &httpResult = result.value();
        Serial.printf("Status code: %d\n", httpResult.statusCode);
        Serial.printf("Status message: %s\n", httpResult.statusMessage.c_str());
        Serial.printf("Response body: %s\n", httpResult.body.c_str());

        Serial.println("Headers:");
        for (const auto &header : httpResult.headers)
        {
            Serial.printf("%s: %s\n", header.first.c_str(), header.second.c_str());
        }

        Serial.println("Cookies:");
        for (const auto &cookie : httpResult.cookies)
        {
            Serial.printf("%s = %s\n", cookie.name.c_str(), cookie.value.c_str());
        }
    }
    else
    {
        const auto &error = result.error();
        Serial.printf("HTTP Error: %s (Error code: %d)\n", error.message.c_str(), static_cast<int>(error.code));
    }
}

void loop()
{
    // Your loop code
    delay(1000);
}
#endif // PIO_UNIT_TESTING