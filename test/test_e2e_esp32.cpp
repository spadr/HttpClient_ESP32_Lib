#ifndef NATIVE_TEST
#include <Arduino.h>
#include <WiFi.h>
#include <unity.h>
#include "Config.h"

#include "../src/HttpClient.h"
#include "../src/core/Request.h"

using namespace canaspad;

// E2Eテスト関数の宣言
void run_e2e_real_server_tests();

void setUp(void) {
    // 各テスト前の初期化
}

void tearDown(void) {
    // 各テスト後のクリーンアップ
}

void setup() {
    delay(2000); // シリアルモニタ接続待ち
    
    Serial.begin(115200);
    Serial.println("\n=== HttpClient ESP32 Library - E2E Tests ===");
    
    // WiFi接続
    Serial.println("Connecting to WiFi...");
    WiFi.begin(Config::ssid, Config::password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // NTP時刻同期
    Serial.println("Synchronizing time with NTP server...");
    configTime(Config::gmt_offset_sec, Config::daylight_offset_sec, Config::ntp_host);
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time. Retrying...");
        delay(1000);
    }
    Serial.println("Time synchronized:");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    
    // Unity テストフレームワーク開始
    UNITY_BEGIN();
    
    // E2Eテスト実行
    run_e2e_real_server_tests();
    
    UNITY_END();
}

void loop() {
    delay(1000);
}

// E2Eテスト実装
void test_basic_http_connection() {
    Serial.println("Testing basic HTTP connection...");
    TEST_ASSERT_TRUE(WiFi.status() == WL_CONNECTED);
    
    HttpClient client;
    Request request;
    request.setUrl("https://httpbin.org/get");
    request.setMethod(canaspad::HttpMethod::GET);
    
    auto result = client.send(request);
    TEST_ASSERT_TRUE(result.isSuccess());
    
    if (result.isSuccess()) {
        auto response = result.value();
        TEST_ASSERT_TRUE(response.statusCode == 200);
        Serial.printf("✓ HTTP GET successful: %d\n", response.statusCode);
    } else {
        Serial.printf("✗ HTTP GET failed\n");
        TEST_FAIL();
    }
}

void test_https_connection() {
    Serial.println("Testing HTTPS connection...");
    TEST_ASSERT_TRUE(WiFi.status() == WL_CONNECTED);
    
    HttpClient client;
    Request request;
    request.setUrl("https://httpbin.org/get");
    request.setMethod(canaspad::HttpMethod::GET);
    
    auto result = client.send(request);
    TEST_ASSERT_TRUE(result.isSuccess());
    
    if (result.isSuccess()) {
        auto response = result.value();
        TEST_ASSERT_TRUE(response.statusCode == 200);
        Serial.printf("✓ HTTPS GET successful: %d\n", response.statusCode);
    } else {
        Serial.printf("✗ HTTPS GET failed\n");
        TEST_FAIL();
    }
}

void test_http_post() {
    Serial.println("Testing HTTP POST...");
    TEST_ASSERT_TRUE(WiFi.status() == WL_CONNECTED);
    
    HttpClient client;
    Request request;
    request.setUrl("https://httpbin.org/post");
    request.setMethod(canaspad::HttpMethod::POST);
    request.setBody("{\"test\":\"data\"}");
    request.addHeader("Content-Type", "application/json");
    
    auto result = client.send(request);
    TEST_ASSERT_TRUE(result.isSuccess());
    
    if (result.isSuccess()) {
        auto response = result.value();
        TEST_ASSERT_TRUE(response.statusCode == 200);
        Serial.printf("✓ HTTP POST successful: %d\n", response.statusCode);
    } else {
        Serial.printf("✗ HTTP POST failed\n");
        TEST_FAIL();
    }
}

void run_e2e_real_server_tests() {
    Serial.println("\n=== Running E2E Real Server Tests ===");
    RUN_TEST(test_basic_http_connection);
    RUN_TEST(test_https_connection);
    RUN_TEST(test_http_post);
}

#endif // NATIVE_TEST