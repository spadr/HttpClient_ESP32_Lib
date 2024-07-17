#include <Arduino.h>
#include <WiFi.h>
#include "HttpClient.h"
#include "Config.h"
// #include "ConfigExample.h"

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

    // HttpClient setup
    http::ClientOptions options;
    options.followRedirects = true;
    options.verifySsl = true;
    options.rootCA = isrg_root_x1;
    options.clientCert = client_cert;
    options.clientPrivateKey = client_key;

    Serial.println("Creating HttpClient");

    http::HttpClient client(options);

    Serial.println("HttpClient created");

    Serial.println("CA certificate set");

    // Request setup
    http::Request request;

    Serial.println("Creating request");
    request.setUrl(api_url)
        .setMethod(http::Request::Method::GET);

    Serial.println("Request created");

    try
    {
        // Send request
        auto futureResponse = client.send(request);
        Serial.println("Request sent main");

        auto response = futureResponse.get();
        Serial.println("Response received");

        Serial.printf("Status code: %d\n", response->getStatusCode());
        Serial.printf("Response body: %s\n", response->getBody().c_str());
    }
    catch (const http::HttpError &e)
    {
        Serial.printf("HTTP Error: %s (Error code: %d)\n", e.what(), static_cast<int>(e.getErrorCode()));
    }
    catch (const std::exception &e)
    {
        Serial.printf("Error: %s\n", e.what());
    }
}

void loop()
{
    // Your loop code
    delay(1000);
}