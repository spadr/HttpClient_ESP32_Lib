#ifdef NATIVE_TEST
#include "../../helpers/TestEnvironment.h"
#define ARDUINO_ARCH_NATIVE
#endif

#include <unity.h>
#include "../../../src/HttpClient.h"
#include "../../../src/core/Request.h"
#include "../../helpers/AssertHelpers.h"
#include "../../helpers/TestEnvironment.h"

using namespace canaspad;

void setUp(void) {
    // テスト前の初期化
}

void tearDown(void) {
    // テスト後のクリーンアップ
}

void test_mockserver_get_request() {
    if (!TestEnvironment::isMockServerAvailable()) {
        TEST_IGNORE_MESSAGE("MockServer not available");
        return;
    }
    
    ClientOptions options;
    options.verifySsl = false;
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("http://localhost:1080/api/test")
           .setMethod(HttpMethod::GET);
    
    auto result = client.send(request);
    
    TestHelpers::assertResultSuccess(result, "MockServer GET request should succeed");
    TestHelpers::assertHttpStatus(result.value(), 200, "Should return 200 OK");
    TestHelpers::assertContains(result.value().body, "Hello from MockServer", "Response should contain expected message");
}

void test_mockserver_post_request() {
    if (!TestEnvironment::isMockServerAvailable()) {
        TEST_IGNORE_MESSAGE("MockServer not available");
        return;
    }
    
    ClientOptions options;
    options.verifySsl = false;
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("http://localhost:1080/api/users")
           .setMethod(HttpMethod::POST)
           .setBody("{\"name\":\"John\",\"email\":\"john@example.com\"}")
           .addHeader("Content-Type", "application/json");
    
    auto result = client.send(request);
    
    TestHelpers::assertResultSuccess(result, "MockServer POST request should succeed");
    TestHelpers::assertHttpStatus(result.value(), 201, "Should return 201 Created");
}

void test_mockserver_bearer_authentication() {
    if (!TestEnvironment::isMockServerAvailable()) {
        TEST_IGNORE_MESSAGE("MockServer not available");
        return;
    }
    
    ClientOptions options;
    options.authType = AuthType::Bearer;
    options.bearerToken = "valid-token";
    options.verifySsl = false;
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("http://localhost:1080/api/auth")
           .setMethod(HttpMethod::POST);
    
    auto result = client.send(request);
    
    TestHelpers::assertResultSuccess(result, "Bearer authentication should succeed");
    TestHelpers::assertHttpStatus(result.value(), 200, "Should return 200 OK");
    TestHelpers::assertContains(result.value().body, "authenticated", "Response should indicate authentication success");
}

void test_mockserver_unauthorized_request() {
    if (!TestEnvironment::isMockServerAvailable()) {
        TEST_IGNORE_MESSAGE("MockServer not available");
        return;
    }
    
    ClientOptions options;
    options.verifySsl = false;
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("http://localhost:1080/api/auth")
           .setMethod(HttpMethod::POST);
    // No authentication headers
    
    auto result = client.send(request);
    
    TestHelpers::assertResultSuccess(result, "HTTP request should succeed even if unauthorized");
    TestHelpers::assertHttpStatus(result.value(), 401, "Should return 401 Unauthorized");
}

void test_mockserver_timeout_handling() {
    if (!TestEnvironment::isMockServerAvailable()) {
        TEST_IGNORE_MESSAGE("MockServer not available");
        return;
    }
    
    ClientOptions options;
    options.verifySsl = false;
    HttpClient client(options, false);
    client.setReadTimeout(std::chrono::seconds(2)); // 2秒でタイムアウト
    
    Request request;
    request.setUrl("http://localhost:1080/api/timeout"); // 5秒遅延
    
    auto result = client.send(request);
    
    TestHelpers::assertResultError(result, ErrorCode::Timeout, "Should timeout after 2 seconds");
}

void test_mockserver_large_response() {
    if (!TestEnvironment::isMockServerAvailable()) {
        TEST_IGNORE_MESSAGE("MockServer not available");
        return;
    }
    
    ClientOptions options;
    options.verifySsl = false;
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("http://localhost:1080/api/large")
           .setMethod(HttpMethod::GET);
    
    auto result = client.send(request);
    
    TestHelpers::assertResultSuccess(result, "Large response should be handled correctly");
    TestHelpers::assertHttpStatus(result.value(), 200, "Should return 200 OK");
    TEST_ASSERT_GREATER_THAN_INT(1000, result.value().body.length()); // At least 1KB
}

void test_mockserver_custom_headers() {
    if (!TestEnvironment::isMockServerAvailable()) {
        TEST_IGNORE_MESSAGE("MockServer not available");
        return;
    }
    
    ClientOptions options;
    options.verifySsl = false;
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("http://localhost:1080/api/test")
           .setMethod(HttpMethod::GET)
           .addHeader("X-Custom-Header", "test-value")
           .addHeader("User-Agent", "HttpClient_ESP32_Lib/1.0");
    
    auto result = client.send(request);
    
    TestHelpers::assertResultSuccess(result, "Request with custom headers should succeed");
    TestHelpers::assertHttpStatus(result.value(), 200, "Should return 200 OK");
}

void test_mockserver_multipart_form_data() {
    if (!TestEnvironment::isMockServerAvailable()) {
        TEST_IGNORE_MESSAGE("MockServer not available");
        return;
    }
    
    ClientOptions options;
    options.verifySsl = false;
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("http://localhost:1080/api/upload")
           .setMethod(HttpMethod::POST)
           .setMultipartFormData({
               {"name", "John Doe"},
               {"age", "30"},
               {"file", "file_content_data"}
           });
    
    auto result = client.send(request);
    
    TestHelpers::assertResultSuccess(result, "Multipart form data should be sent successfully");
    TestHelpers::assertHttpStatus(result.value(), 200, "Should return 200 OK");
}

void test_mockserver_error_responses() {
    if (!TestEnvironment::isMockServerAvailable()) {
        TEST_IGNORE_MESSAGE("MockServer not available");
        return;
    }
    
    ClientOptions options;
    options.verifySsl = false;
    HttpClient client(options, false);
    
    // Test 404 Not Found
    Request request404;
    request404.setUrl("http://localhost:1080/api/notfound")
              .setMethod(HttpMethod::GET);
    
    auto result404 = client.send(request404);
    
    TestHelpers::assertResultSuccess(result404, "HTTP 404 is a successful HTTP response");
    TestHelpers::assertHttpStatus(result404.value(), 404, "Should return 404 Not Found");
    
    // Test 500 Internal Server Error
    Request request500;
    request500.setUrl("http://localhost:1080/api/error")
              .setMethod(HttpMethod::GET);
    
    auto result500 = client.send(request500);
    
    TestHelpers::assertResultSuccess(result500, "HTTP 500 is a successful HTTP response");
    TestHelpers::assertHttpStatus(result500.value(), 500, "Should return 500 Internal Server Error");
}

void test_mockserver_redirect_handling() {
    if (!TestEnvironment::isMockServerAvailable()) {
        TEST_IGNORE_MESSAGE("MockServer not available");
        return;
    }
    
    ClientOptions options;
    options.followRedirects = true;
    options.maxRedirects = 5;
    options.verifySsl = false;
    HttpClient client(options, false);
    
    Request request;
    request.setUrl("http://localhost:1080/api/redirect")
           .setMethod(HttpMethod::GET);
    
    auto result = client.send(request);
    
    TestHelpers::assertResultSuccess(result, "Redirect should be followed successfully");
    TestHelpers::assertHttpStatus(result.value(), 200, "Final response should be 200 OK");
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_mockserver_get_request);
    RUN_TEST(test_mockserver_post_request);
    RUN_TEST(test_mockserver_bearer_authentication);
    RUN_TEST(test_mockserver_unauthorized_request);
    RUN_TEST(test_mockserver_timeout_handling);
    RUN_TEST(test_mockserver_large_response);
    RUN_TEST(test_mockserver_custom_headers);
    RUN_TEST(test_mockserver_multipart_form_data);
    RUN_TEST(test_mockserver_error_responses);
    RUN_TEST(test_mockserver_redirect_handling);
    
    return UNITY_END();
}