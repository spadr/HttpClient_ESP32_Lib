#ifdef NATIVE_TEST
#include "../unit/native_arduino_mock.h"
#define ARDUINO_ARCH_NATIVE
#endif

#include <unity.h>
#include "../../src/HttpClient.h"
#include "../../src/core/Request.h"
#include <iostream>
#include <curl/curl.h>

using namespace canaspad;

// MockServer configuration
const char* MOCK_SERVER_URL = "http://localhost:1080";
const char* MOCK_SERVER_HOST = "localhost";
const int MOCK_SERVER_PORT = 1080;

// Helper to setup MockServer expectations via REST API
bool setupMockServerExpectation(const std::string& method, const std::string& path, 
                               int statusCode, const std::string& responseBody) {
    CURL *curl;
    CURLcode res;
    
    curl = curl_easy_init();
    if(curl) {
        std::string url = std::string(MOCK_SERVER_URL) + "/mockserver/expectation";
        std::string jsonData = R"({
            "httpRequest": {
                "method": ")" + method + R"(",
                "path": ")" + path + R"("
            },
            "httpResponse": {
                "statusCode": )" + std::to_string(statusCode) + R"(,
                "headers": {
                    "Content-Type": ["application/json"]
                },
                "body": ")" + responseBody + R"("
            }
        })";
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        return res == CURLE_OK;
    }
    return false;
}

void setUp(void) {
    // Reset MockServer before each test
    CURL *curl = curl_easy_init();
    if(curl) {
        std::string url = std::string(MOCK_SERVER_URL) + "/mockserver/reset";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
}

void tearDown(void) {
    // テスト後のクリーンアップ
}

void test_http_get_success() {
    // Setup MockServer expectation
    TEST_ASSERT_TRUE(setupMockServerExpectation("GET", "/api/test", 200, "Hello World"));
    
    // Test HTTP GET request
    ClientOptions options;
    HttpClient client(options, false); // Use real connection
    
    Request request;
    request.setUrl("http://localhost:1080/api/test")
           .setMethod(HttpMethod::GET);
    
    auto result = client.send(request);
    
    TEST_ASSERT_TRUE(result.isSuccess());
    auto httpResult = result.value();
    TEST_ASSERT_EQUAL(200, httpResult.statusCode);
    TEST_ASSERT_EQUAL_STRING("Hello World", httpResult.body.c_str());
}

void test_http_post_with_body() {
    // Setup MockServer expectation
    TEST_ASSERT_TRUE(setupMockServerExpectation("POST", "/api/users", 201, "{\"id\":123,\"status\":\"created\"}"));
    
    ClientOptions options;
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("http://localhost:1080/api/users")
           .setMethod(HttpMethod::POST)
           .setBody("{\"name\":\"John\",\"email\":\"john@example.com\"}")
           .addHeader("Content-Type", "application/json");
    
    auto result = client.send(request);
    
    TEST_ASSERT_TRUE(result.isSuccess());
    auto httpResult = result.value();
    TEST_ASSERT_EQUAL(201, httpResult.statusCode);
    TEST_ASSERT_TRUE(httpResult.body.find("\"id\":123") != std::string::npos);
}

void test_http_redirect_handling() {
    // Setup redirect chain: /redirect -> /final
    CURL *curl = curl_easy_init();
    if(curl) {
        std::string url = std::string(MOCK_SERVER_URL) + "/mockserver/expectation";
        std::string redirectJson = R"({
            "httpRequest": {
                "method": "GET",
                "path": "/redirect"
            },
            "httpResponse": {
                "statusCode": 302,
                "headers": {
                    "Location": ["http://localhost:1080/final"]
                }
            }
        })";
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, redirectJson.c_str());
        struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    TEST_ASSERT_TRUE(setupMockServerExpectation("GET", "/final", 200, "Final destination"));
    
    ClientOptions options;
    options.followRedirects = true;
    options.maxRedirects = 5;
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("http://localhost:1080/redirect")
           .setMethod(HttpMethod::GET);
    
    auto result = client.send(request);
    
    TEST_ASSERT_TRUE(result.isSuccess());
    auto httpResult = result.value();
    TEST_ASSERT_EQUAL(200, httpResult.statusCode);
    TEST_ASSERT_EQUAL_STRING("Final destination", httpResult.body.c_str());
}

void test_http_error_responses() {
    // Test 404 Not Found
    TEST_ASSERT_TRUE(setupMockServerExpectation("GET", "/notfound", 404, "Not Found"));
    
    ClientOptions options;
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("http://localhost:1080/notfound")
           .setMethod(HttpMethod::GET);
    
    auto result = client.send(request);
    
    TEST_ASSERT_TRUE(result.isSuccess()); // HTTP 404 is a successful HTTP response
    auto httpResult = result.value();
    TEST_ASSERT_EQUAL(404, httpResult.statusCode);
}

void test_http_timeout_handling() {
    // Setup delayed response (5 seconds)
    CURL *curl = curl_easy_init();
    if(curl) {
        std::string url = std::string(MOCK_SERVER_URL) + "/mockserver/expectation";
        std::string delayJson = R"({
            "httpRequest": {
                "method": "GET",
                "path": "/slow"
            },
            "httpResponse": {
                "statusCode": 200,
                "body": "Slow response",
                "delay": {
                    "timeUnit": "SECONDS",
                    "value": 5
                }
            }
        })";
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, delayJson.c_str());
        struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    ClientOptions options;
    HttpClient client(options, false);
    
    // Set short timeout
    HttpClient::Timeouts timeouts;
    timeouts.read = std::chrono::milliseconds(2000); // 2 seconds
    client.setTimeouts(timeouts);
    
    Request request;
    request.setUrl("http://localhost:1080/slow")
           .setMethod(HttpMethod::GET);
    
    auto result = client.send(request);
    
    // Should timeout
    TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(ErrorCode::Timeout, result.error().code);
}

void test_http_large_response() {
    // Generate large response body (10KB)
    std::string largeBody(10240, 'x');
    TEST_ASSERT_TRUE(setupMockServerExpectation("GET", "/large", 200, largeBody));
    
    ClientOptions options;
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("http://localhost:1080/large")
           .setMethod(HttpMethod::GET);
    
    auto result = client.send(request);
    
    TEST_ASSERT_TRUE(result.isSuccess());
    auto httpResult = result.value();
    TEST_ASSERT_EQUAL(200, httpResult.statusCode);
    TEST_ASSERT_EQUAL(10240, httpResult.body.length());
}

void setup() {
    // Check if MockServer is running
    CURL *curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, MOCK_SERVER_URL);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if(res != CURLE_OK) {
            std::cout << "MockServer not available at " << MOCK_SERVER_URL << std::endl;
            std::cout << "Please start MockServer: docker run -d -p 1080:1080 mockserver/mockserver" << std::endl;
            return;
        }
    }
    
    UNITY_BEGIN();
    
    RUN_TEST(test_http_get_success);
    RUN_TEST(test_http_post_with_body);
    RUN_TEST(test_http_redirect_handling);
    RUN_TEST(test_http_error_responses);
    RUN_TEST(test_http_timeout_handling);
    RUN_TEST(test_http_large_response);
    
    UNITY_END();
}

void loop() {
    // Empty loop for PlatformIO compatibility
}