#include <unity.h>
#include <WiFi.h>
#include "time.h"
#include "../../src/HttpClient.h"
#include "../../src/core/Request.h"

// Test configuration
#ifndef CONFIG_H_EXISTS
#error "Config.h not found. Please copy src/ConfigExample.h to src/Config.h and configure"
#endif
#include "../../src/Config.h"

using namespace canaspad;

void setUp(void) {
    // Ensure WiFi is connected before each test
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconnecting to WiFi...");
        WiFi.begin(Config::ssid, Config::password);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() != WL_CONNECTED) {
            TEST_FAIL_MESSAGE("WiFi connection failed");
        }
        
        Serial.println("\nWiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
}

void tearDown(void) {
    // Clean up after each test
    delay(100);
}

void test_httpbin_get_request() {
    Serial.println("Testing HTTP GET with httpbin.org");
    
    ClientOptions options;
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("http://httpbin.org/get")
           .setMethod(HttpMethod::GET)
           .addHeader("User-Agent", "HttpClient-ESP32/1.0")
           .addHeader("Accept", "application/json");
    
    auto result = client.send(request);
    
    TEST_ASSERT_TRUE_MESSAGE(result.isSuccess(), "HTTP request should succeed");
    
    auto httpResult = result.value();
    TEST_ASSERT_EQUAL_MESSAGE(200, httpResult.statusCode, "Should return 200 OK");
    TEST_ASSERT_TRUE_MESSAGE(httpResult.body.length() > 0, "Response body should not be empty");
    
    // Check if response contains expected JSON structure
    TEST_ASSERT_TRUE_MESSAGE(httpResult.body.find("\"url\"") != std::string::npos, 
                            "Response should contain URL field");
    TEST_ASSERT_TRUE_MESSAGE(httpResult.body.find("\"headers\"") != std::string::npos, 
                            "Response should contain headers field");
    
    Serial.printf("Response length: %zu bytes\n", httpResult.body.length());
    Serial.printf("Status: %d %s\n", httpResult.statusCode, httpResult.statusMessage.c_str());
}

void test_httpbin_post_request() {
    Serial.println("Testing HTTP POST with httpbin.org");
    
    ClientOptions options;
    HttpClient client(options, false);
    
    std::string postData = "{\"test\":\"data\",\"timestamp\":\"" + 
                          std::to_string(millis()) + "\"}";
    
    Request request;
    request.setUrl("http://httpbin.org/post")
           .setMethod(HttpMethod::POST)
           .addHeader("Content-Type", "application/json")
           .addHeader("User-Agent", "HttpClient-ESP32/1.0")
           .setBody(postData);
    
    auto result = client.send(request);
    
    TEST_ASSERT_TRUE_MESSAGE(result.isSuccess(), "POST request should succeed");
    
    auto httpResult = result.value();
    TEST_ASSERT_EQUAL_MESSAGE(200, httpResult.statusCode, "Should return 200 OK");
    
    // Verify that our posted data is echoed back
    TEST_ASSERT_TRUE_MESSAGE(httpResult.body.find("\"test\":\"data\"") != std::string::npos,
                            "Posted data should be echoed back");
    
    Serial.printf("POST response length: %zu bytes\n", httpResult.body.length());
}

void test_https_request() {
    Serial.println("Testing HTTPS request");
    
    ClientOptions options;
    options.verifySsl = true; // Enable SSL verification
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("https://httpbin.org/get")
           .setMethod(HttpMethod::GET)
           .addHeader("User-Agent", "HttpClient-ESP32/1.0");
    
    auto result = client.send(request);
    
    TEST_ASSERT_TRUE_MESSAGE(result.isSuccess(), "HTTPS request should succeed");
    
    auto httpResult = result.value();
    TEST_ASSERT_EQUAL_MESSAGE(200, httpResult.statusCode, "Should return 200 OK");
    
    Serial.printf("HTTPS response length: %zu bytes\n", httpResult.body.length());
}

void test_request_timeout() {
    Serial.println("Testing request timeout");
    
    ClientOptions options;
    HttpClient client(options, false);
    
    // Set a very short timeout
    HttpClient::Timeouts timeouts;
    timeouts.connect = std::chrono::milliseconds(1000);
    timeouts.read = std::chrono::milliseconds(2000);
    client.setTimeouts(timeouts);
    
    Request request;
    request.setUrl("http://httpbin.org/delay/10") // 10 second delay
           .setMethod(HttpMethod::GET);
    
    auto result = client.send(request);
    
    // Should timeout
    TEST_ASSERT_TRUE_MESSAGE(result.isError(), "Request should timeout");
    TEST_ASSERT_EQUAL_MESSAGE(ErrorCode::Timeout, result.error().code, 
                             "Error should be timeout");
    
    Serial.println("Timeout test completed successfully");
}

void test_redirect_following() {
    Serial.println("Testing redirect following");
    
    ClientOptions options;
    options.followRedirects = true;
    options.maxRedirects = 5;
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("http://httpbin.org/redirect/2") // 2 redirects
           .setMethod(HttpMethod::GET);
    
    auto result = client.send(request);
    
    TEST_ASSERT_TRUE_MESSAGE(result.isSuccess(), "Redirect should be followed successfully");
    
    auto httpResult = result.value();
    TEST_ASSERT_EQUAL_MESSAGE(200, httpResult.statusCode, "Final response should be 200 OK");
    
    Serial.println("Redirect test completed successfully");
}

void test_large_response() {
    Serial.println("Testing large response handling");
    
    ClientOptions options;
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("http://httpbin.org/bytes/10240") // 10KB response
           .setMethod(HttpMethod::GET);
    
    auto result = client.send(request);
    
    TEST_ASSERT_TRUE_MESSAGE(result.isSuccess(), "Large response request should succeed");
    
    auto httpResult = result.value();
    TEST_ASSERT_EQUAL_MESSAGE(200, httpResult.statusCode, "Should return 200 OK");
    TEST_ASSERT_EQUAL_MESSAGE(10240, httpResult.body.length(), "Response should be exactly 10KB");
    
    Serial.printf("Large response test: received %zu bytes\n", httpResult.body.length());
}

void test_memory_usage() {
    Serial.println("Testing memory usage");
    
    size_t freeHeapBefore = ESP.getFreeHeap();
    Serial.printf("Free heap before test: %zu bytes\n", freeHeapBefore);
    
    {
        ClientOptions options;
        HttpClient client(options, false);
        
        Request request;
        request.setUrl("http://httpbin.org/get")
               .setMethod(HttpMethod::GET);
        
        auto result = client.send(request);
        TEST_ASSERT_TRUE_MESSAGE(result.isSuccess(), "Memory test request should succeed");
        
        // Force cleanup
        result = Result<HttpResult>(ErrorInfo(ErrorCode::None, ""));
    }
    
    // Allow some time for cleanup
    delay(100);
    
    size_t freeHeapAfter = ESP.getFreeHeap();
    Serial.printf("Free heap after test: %zu bytes\n", freeHeapAfter);
    
    // We should not have leaked more than 1KB
    size_t heapDiff = freeHeapBefore - freeHeapAfter;
    TEST_ASSERT_TRUE_MESSAGE(heapDiff < 1024, "Memory leak should be minimal (<1KB)");
    
    Serial.printf("Memory usage difference: %zu bytes\n", heapDiff);
}

void test_performance_benchmark() {
    Serial.println("Running performance benchmark");
    
    ClientOptions options;
    HttpClient client(options, false);
    
    const int numRequests = 5;
    unsigned long totalTime = 0;
    int successCount = 0;
    
    for (int i = 0; i < numRequests; i++) {
        unsigned long startTime = millis();
        
        Request request;
        request.setUrl("http://httpbin.org/get")
               .setMethod(HttpMethod::GET);
        
        auto result = client.send(request);
        
        unsigned long endTime = millis();
        unsigned long requestTime = endTime - startTime;
        
        if (result.isSuccess()) {
            successCount++;
            totalTime += requestTime;
            Serial.printf("Request %d: %lu ms\n", i + 1, requestTime);
        } else {
            Serial.printf("Request %d: FAILED\n", i + 1);
        }
        
        delay(100); // Brief pause between requests
    }
    
    TEST_ASSERT_TRUE_MESSAGE(successCount >= numRequests * 0.8, 
                            "At least 80% of requests should succeed");
    
    if (successCount > 0) {
        unsigned long avgTime = totalTime / successCount;
        Serial.printf("Average request time: %lu ms\n", avgTime);
        Serial.printf("Success rate: %d/%d (%.1f%%)\n", 
                     successCount, numRequests, 
                     (float)successCount * 100.0 / numRequests);
        
        // Performance target: average request should complete within 5 seconds
        TEST_ASSERT_TRUE_MESSAGE(avgTime < 5000, 
                                "Average request time should be under 5 seconds");
    }
}

void setup() {
    delay(2000); // Wait for serial monitor
    
    Serial.begin(115200);
    Serial.println("Starting ESP32 E2E Tests");
    
    // Connect to WiFi
    Serial.printf("Connecting to WiFi: %s\n", Config::ssid);
    WiFi.begin(Config::ssid, Config::password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nFailed to connect to WiFi");
        Serial.println("Please check your Config.h settings");
        return;
    }
    
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Sync time with NTP
    Serial.println("Synchronizing time...");
    configTime(Config::gmt_offset_sec, Config::daylight_offset_sec, Config::ntp_host);
    
    struct tm timeinfo;
    int timeAttempts = 0;
    while (!getLocalTime(&timeinfo) && timeAttempts < 10) {
        Serial.println("Failed to obtain time, retrying...");
        delay(1000);
        timeAttempts++;
    }
    
    if (timeAttempts >= 10) {
        Serial.println("Warning: Could not sync time");
    } else {
        Serial.println("Time synchronized");
        Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    }
    
    // Run tests
    UNITY_BEGIN();
    
    RUN_TEST(test_httpbin_get_request);
    RUN_TEST(test_httpbin_post_request);
    RUN_TEST(test_https_request);
    RUN_TEST(test_request_timeout);
    RUN_TEST(test_redirect_following);
    RUN_TEST(test_large_response);
    RUN_TEST(test_memory_usage);
    RUN_TEST(test_performance_benchmark);
    
    UNITY_END();
}

void loop() {
    delay(1000);
}