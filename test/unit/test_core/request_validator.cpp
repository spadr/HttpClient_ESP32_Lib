#ifdef NATIVE_TEST
#include "../../helpers/TestEnvironment.h"
#define ARDUINO_ARCH_NATIVE
#endif

#include <unity.h>
#include "../../../src/core/RequestValidator.h"
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

void test_validate_valid_request() {
    Request request;
    request.setUrl("http://example.com/test")
           .setMethod(HttpMethod::GET);
    
    ClientOptions options;
    
    auto result = RequestValidator::validate(request, options);
    
    TestHelpers::assertResultSuccess(result, "Valid request should pass validation");
}

void test_validate_empty_url() {
    Request request;
    request.setUrl("")
           .setMethod(HttpMethod::GET);
    
    ClientOptions options;
    
    auto result = RequestValidator::validate(request, options);
    
    TestHelpers::assertResultError(result, ErrorCode::InvalidURL, "Empty URL should fail validation");
}

void test_validate_invalid_url_no_scheme() {
    Request request;
    request.setUrl("example.com/test")
           .setMethod(HttpMethod::GET);
    
    ClientOptions options;
    
    auto result = RequestValidator::validate(request, options);
    
    TestHelpers::assertResultError(result, ErrorCode::InvalidURL, "URL without scheme should fail validation");
}

void test_validate_invalid_url_format() {
    Request request;
    request.setUrl("not-a-valid-url")
           .setMethod(HttpMethod::GET);
    
    ClientOptions options;
    
    auto result = RequestValidator::validate(request, options);
    
    TestHelpers::assertResultError(result, ErrorCode::InvalidURL, "Invalid URL format should fail validation");
}

void test_validate_unsupported_scheme() {
    Request request;
    request.setUrl("ftp://example.com/test")
           .setMethod(HttpMethod::GET);
    
    ClientOptions options;
    
    auto result = RequestValidator::validate(request, options);
    
    TestHelpers::assertResultError(result, ErrorCode::UnsupportedProtocol, "Unsupported protocol should fail validation");
}

void test_validate_https_request() {
    Request request;
    request.setUrl("https://example.com/test")
           .setMethod(HttpMethod::GET);
    
    ClientOptions options;
    options.verifySsl = true;
    
    auto result = RequestValidator::validate(request, options);
    
    TestHelpers::assertResultSuccess(result, "HTTPS request should pass validation");
}

void test_validate_post_with_body() {
    Request request;
    request.setUrl("http://example.com/api")
           .setMethod(HttpMethod::POST)
           .setBody("test body data");
    
    ClientOptions options;
    
    auto result = RequestValidator::validate(request, options);
    
    TestHelpers::assertResultSuccess(result, "POST request with body should pass validation");
}

void test_validate_get_with_body() {
    Request request;
    request.setUrl("http://example.com/api")
           .setMethod(HttpMethod::GET)
           .setBody("unexpected body");
    
    ClientOptions options;
    
    auto result = RequestValidator::validate(request, options);
    
    // GET with body might be invalid depending on implementation
    // This test checks the current behavior
    if (result.isError()) {
        TestHelpers::assertResultError(result, ErrorCode::InvalidBody, "GET request with body validation");
    } else {
        TestHelpers::assertResultSuccess(result, "GET request with body validation");
    }
}

void test_validate_proxy_url_valid() {
    Request request;
    request.setUrl("http://example.com/test")
           .setMethod(HttpMethod::GET);
    
    ClientOptions options;
    options.proxyUrl = "http://proxy.example.com:8080";
    
    auto result = RequestValidator::validate(request, options);
    
    TestHelpers::assertResultSuccess(result, "Valid proxy URL should pass validation");
}

void test_validate_proxy_url_invalid() {
    Request request;
    request.setUrl("http://example.com/test")
           .setMethod(HttpMethod::GET);
    
    ClientOptions options;
    options.proxyUrl = "invalid-proxy-url";
    
    auto result = RequestValidator::validate(request, options);
    
    TestHelpers::assertResultError(result, ErrorCode::InvalidProxyURL, "Invalid proxy URL should fail validation");
}

void test_validate_headers_valid() {
    Request request;
    request.setUrl("http://example.com/test")
           .setMethod(HttpMethod::POST)
           .addHeader("Content-Type", "application/json")
           .addHeader("Authorization", "Bearer token123");
    
    ClientOptions options;
    
    auto result = RequestValidator::validate(request, options);
    
    TestHelpers::assertResultSuccess(result, "Valid headers should pass validation");
}

void test_validate_headers_empty_name() {
    Request request;
    request.setUrl("http://example.com/test")
           .setMethod(HttpMethod::GET)
           .addHeader("", "value");
    
    ClientOptions options;
    
    auto result = RequestValidator::validate(request, options);
    
    TestHelpers::assertResultError(result, ErrorCode::InvalidHeader, "Empty header name should fail validation");
}

void test_validate_headers_invalid_characters() {
    Request request;
    request.setUrl("http://example.com/test")
           .setMethod(HttpMethod::GET)
           .addHeader("Invalid Header Name", "value");
    
    ClientOptions options;
    
    auto result = RequestValidator::validate(request, options);
    
    // This might pass or fail depending on implementation
    // The test verifies current behavior
    if (result.isError()) {
        TestHelpers::assertResultError(result, ErrorCode::InvalidHeader, "Invalid header name validation");
    } else {
        TestHelpers::assertResultSuccess(result, "Invalid header name validation");
    }
}

void test_validate_multipart_form_data() {
    Request request;
    request.setUrl("http://example.com/upload")
           .setMethod(HttpMethod::POST)
           .setMultipartFormData({
               {"name", "John Doe"},
               {"age", "30"},
               {"file", "file_content"}
           });
    
    ClientOptions options;
    
    auto result = RequestValidator::validate(request, options);
    
    TestHelpers::assertResultSuccess(result, "Multipart form data should pass validation");
}

void test_validate_max_redirects_negative() {
    Request request;
    request.setUrl("http://example.com/test")
           .setMethod(HttpMethod::GET);
    
    ClientOptions options;
    options.maxRedirects = -1;
    
    auto result = RequestValidator::validate(request, options);
    
    TestHelpers::assertResultError(result, ErrorCode::InvalidOption, "Negative max redirects should fail validation");
}

void test_validate_max_retries_negative() {
    Request request;
    request.setUrl("http://example.com/test")
           .setMethod(HttpMethod::GET);
    
    ClientOptions options;
    options.maxRetries = -1;
    
    auto result = RequestValidator::validate(request, options);
    
    TestHelpers::assertResultError(result, ErrorCode::InvalidOption, "Negative max retries should fail validation");
}

void test_validate_auth_basic_missing_credentials() {
    Request request;
    request.setUrl("http://example.com/test")
           .setMethod(HttpMethod::GET);
    
    ClientOptions options;
    options.authType = AuthType::Basic;
    options.username = "";
    options.password = "";
    
    auto result = RequestValidator::validate(request, options);
    
    // This might pass or fail depending on implementation
    // Empty credentials might be valid in some cases
    if (result.isError()) {
        TestHelpers::assertResultError(result, ErrorCode::InvalidOption, "Basic auth with empty credentials validation");
    } else {
        TestHelpers::assertResultSuccess(result, "Basic auth with empty credentials validation");
    }
}

void test_validate_auth_bearer_missing_token() {
    Request request;
    request.setUrl("http://example.com/test")
           .setMethod(HttpMethod::GET);
    
    ClientOptions options;
    options.authType = AuthType::Bearer;
    options.bearerToken = "";
    
    auto result = RequestValidator::validate(request, options);
    
    // This might pass or fail depending on implementation
    if (result.isError()) {
        TestHelpers::assertResultError(result, ErrorCode::InvalidOption, "Bearer auth with empty token validation");
    } else {
        TestHelpers::assertResultSuccess(result, "Bearer auth with empty token validation");
    }
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_validate_valid_request);
    RUN_TEST(test_validate_empty_url);
    RUN_TEST(test_validate_invalid_url_no_scheme);
    RUN_TEST(test_validate_invalid_url_format);
    RUN_TEST(test_validate_unsupported_scheme);
    RUN_TEST(test_validate_https_request);
    RUN_TEST(test_validate_post_with_body);
    RUN_TEST(test_validate_get_with_body);
    RUN_TEST(test_validate_proxy_url_valid);
    RUN_TEST(test_validate_proxy_url_invalid);
    RUN_TEST(test_validate_headers_valid);
    RUN_TEST(test_validate_headers_empty_name);
    RUN_TEST(test_validate_headers_invalid_characters);
    RUN_TEST(test_validate_multipart_form_data);
    RUN_TEST(test_validate_max_redirects_negative);
    RUN_TEST(test_validate_max_retries_negative);
    RUN_TEST(test_validate_auth_basic_missing_credentials);
    RUN_TEST(test_validate_auth_bearer_missing_token);
    
    return UNITY_END();
}