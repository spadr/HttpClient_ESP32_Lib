#ifdef NATIVE_TEST
#include "../../helpers/TestEnvironment.h"
#define ARDUINO_ARCH_NATIVE
#endif

#include "../../helpers/simple_unity.h"
#include "../../../src/auth/Auth.h"
#include "../../../src/core/Request.h"
#include "../../../src/core/CommonTypes.h"
#include "../../helpers/AssertHelpers.h"

using namespace canaspad;

void setUp(void) {
    // テスト前の初期化
}

void tearDown(void) {
    // テスト後のクリーンアップ
}

void test_auth_none() {
    ClientOptions options;
    options.authType = AuthType::None;
    
    Auth auth(options);
    Request request;
    request.setUrl("http://example.com/test");
    
    auth.applyAuthentication(request);
    
    // No Authorization header should be added
    const auto& headers = request.getHeaders();
    TEST_ASSERT_TRUE(headers.find("Authorization") == headers.end());
}

void test_auth_basic() {
    ClientOptions options;
    options.authType = AuthType::Basic;
    options.username = "testuser";
    options.password = "testpass";
    
    Auth auth(options);
    Request request;
    request.setUrl("http://example.com/test");
    
    auth.applyAuthentication(request);
    
    const auto& headers = request.getHeaders();
    TEST_ASSERT_TRUE(headers.find("Authorization") != headers.end());
    
    const std::string& authHeader = headers.at("Authorization");
    TEST_ASSERT_TRUE(authHeader.find("Basic ") == 0);
    // Basic認証のエンコードは base64(testuser:testpass) になる
    TestHelpers::assertContains(authHeader, "Basic ", "Basic auth header should start with 'Basic '");
}

void test_auth_bearer() {
    ClientOptions options;
    options.authType = AuthType::Bearer;
    options.bearerToken = "test-token-123";
    
    Auth auth(options);
    Request request;
    request.setUrl("http://example.com/test");
    
    auth.applyAuthentication(request);
    
    const auto& headers = request.getHeaders();
    TEST_ASSERT_TRUE(headers.find("Authorization") != headers.end());
    
    const std::string& authHeader = headers.at("Authorization");
    TEST_ASSERT_EQUAL_STRING("Bearer test-token-123", authHeader.c_str());
}

void test_auth_basic_empty_credentials() {
    ClientOptions options;
    options.authType = AuthType::Basic;
    options.username = "";
    options.password = "";
    
    Auth auth(options);
    Request request;
    request.setUrl("http://example.com/test");
    
    auth.applyAuthentication(request);
    
    const auto& headers = request.getHeaders();
    TEST_ASSERT_TRUE(headers.find("Authorization") != headers.end());
    
    const std::string& authHeader = headers.at("Authorization");
    TEST_ASSERT_TRUE(authHeader.find("Basic ") == 0);
}

void test_auth_bearer_empty_token() {
    ClientOptions options;
    options.authType = AuthType::Bearer;
    options.bearerToken = "";
    
    Auth auth(options);
    Request request;
    request.setUrl("http://example.com/test");
    
    auth.applyAuthentication(request);
    
    const auto& headers = request.getHeaders();
    TEST_ASSERT_TRUE(headers.find("Authorization") != headers.end());
    
    const std::string& authHeader = headers.at("Authorization");
    TEST_ASSERT_EQUAL_STRING("Bearer ", authHeader.c_str());
}

void test_auth_request_modification() {
    ClientOptions options;
    options.authType = AuthType::Bearer;
    options.bearerToken = "test-token";
    
    Auth auth(options);
    Request request;
    request.setUrl("http://example.com/test")
           .setMethod(HttpMethod::POST)
           .setBody("test body")
           .addHeader("Content-Type", "application/json");
    
    auth.applyAuthentication(request);
    
    // Check that existing headers are preserved
    const auto& headers = request.getHeaders();
    TEST_ASSERT_TRUE(headers.find("Content-Type") != headers.end());
    TEST_ASSERT_EQUAL_STRING("application/json", headers.at("Content-Type").c_str());
    
    // Check that Authorization header is added
    TEST_ASSERT_TRUE(headers.find("Authorization") != headers.end());
    TEST_ASSERT_EQUAL_STRING("Bearer test-token", headers.at("Authorization").c_str());
    
    // Check that other request properties are preserved
    TEST_ASSERT_EQUAL_STRING("http://example.com/test", request.getUrl().c_str());
    TEST_ASSERT_EQUAL_STRING("test body", request.getBody().c_str());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpMethod::POST), static_cast<int>(request.getMethod()));
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_auth_none);
    RUN_TEST(test_auth_basic);
    RUN_TEST(test_auth_bearer);
    RUN_TEST(test_auth_basic_empty_credentials);
    RUN_TEST(test_auth_bearer_empty_token);
    RUN_TEST(test_auth_request_modification);
    
    return UNITY_END();
}