#ifdef NATIVE_TEST
#include "../../helpers/TestEnvironment.h"
#define ARDUINO_ARCH_NATIVE
#endif

#include "../../helpers/simple_unity.h"
#include "../../../src/utils/Utils.h"
#include "../../../src/utils/HttpMethod.h"
#include "../../../src/core/HttpResult.h"
#include "../../helpers/AssertHelpers.h"

using namespace canaspad;

void setUp(void) {
    // テスト前の初期化
}

void tearDown(void) {
    // テスト後のクリーンアップ
}

// URL parsing tests
void test_extract_scheme_http() {
    std::string url = "http://example.com/path";
    std::string scheme = Utils::extractScheme(url);
    TEST_ASSERT_EQUAL_STRING("http", scheme.c_str());
}

void test_extract_scheme_https() {
    std::string url = "https://example.com/path";
    std::string scheme = Utils::extractScheme(url);
    TEST_ASSERT_EQUAL_STRING("https", scheme.c_str());
}

void test_extract_scheme_no_scheme() {
    std::string url = "example.com/path";
    std::string scheme = Utils::extractScheme(url);
    TEST_ASSERT_EQUAL_STRING("", scheme.c_str());
}

void test_extract_host_basic() {
    std::string url = "http://example.com/path";
    std::string host = Utils::extractHost(url);
    TEST_ASSERT_EQUAL_STRING("example.com", host.c_str());
}

void test_extract_host_with_port() {
    std::string url = "http://example.com:8080/path";
    std::string host = Utils::extractHost(url);
    TEST_ASSERT_EQUAL_STRING("example.com", host.c_str());
}

void test_extract_host_subdomain() {
    std::string url = "https://api.example.com/v1/users";
    std::string host = Utils::extractHost(url);
    TEST_ASSERT_EQUAL_STRING("api.example.com", host.c_str());
}

void test_extract_port_default_http() {
    std::string url = "http://example.com/path";
    int port = Utils::extractPort(url);
    TEST_ASSERT_EQUAL_INT(80, port);
}

void test_extract_port_default_https() {
    std::string url = "https://example.com/path";
    int port = Utils::extractPort(url);
    TEST_ASSERT_EQUAL_INT(443, port);
}

void test_extract_port_custom() {
    std::string url = "http://example.com:8080/path";
    int port = Utils::extractPort(url);
    TEST_ASSERT_EQUAL_INT(8080, port);
}

void test_extract_path_basic() {
    std::string url = "http://example.com/api/users";
    std::string path = Utils::extractPath(url);
    TEST_ASSERT_EQUAL_STRING("/api/users", path.c_str());
}

void test_extract_path_root() {
    std::string url = "http://example.com/";
    std::string path = Utils::extractPath(url);
    TEST_ASSERT_EQUAL_STRING("/", path.c_str());
}

void test_extract_path_no_path() {
    std::string url = "http://example.com";
    std::string path = Utils::extractPath(url);
    TEST_ASSERT_EQUAL_STRING("/", path.c_str());
}

void test_extract_path_with_query() {
    std::string url = "http://example.com/api/users?id=123&name=test";
    std::string path = Utils::extractPath(url);
    TEST_ASSERT_EQUAL_STRING("/api/users?id=123&name=test", path.c_str());
}

void test_extract_base_url() {
    std::string url = "http://example.com/api/users";
    std::string baseUrl = Utils::extractBaseUrl(url);
    TEST_ASSERT_EQUAL_STRING("http://example.com", baseUrl.c_str());
}

void test_extract_base_url_with_port() {
    std::string url = "https://example.com:8080/api/users";
    std::string baseUrl = Utils::extractBaseUrl(url);
    TEST_ASSERT_EQUAL_STRING("https://example.com:8080", baseUrl.c_str());
}

// HTTP Method tests
void test_http_method_to_string() {
    TEST_ASSERT_EQUAL_STRING("GET", httpMethodToString(HttpMethod::GET).c_str());
    TEST_ASSERT_EQUAL_STRING("POST", httpMethodToString(HttpMethod::POST).c_str());
    TEST_ASSERT_EQUAL_STRING("PUT", httpMethodToString(HttpMethod::PUT).c_str());
    TEST_ASSERT_EQUAL_STRING("DELETE", httpMethodToString(HttpMethod::DELETE).c_str());
    TEST_ASSERT_EQUAL_STRING("PATCH", httpMethodToString(HttpMethod::PATCH).c_str());
    TEST_ASSERT_EQUAL_STRING("HEAD", httpMethodToString(HttpMethod::HEAD).c_str());
    TEST_ASSERT_EQUAL_STRING("OPTIONS", httpMethodToString(HttpMethod::OPTIONS).c_str());
}

void test_string_to_http_method() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpMethod::GET), static_cast<int>(stringToHttpMethod("GET")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpMethod::POST), static_cast<int>(stringToHttpMethod("POST")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpMethod::PUT), static_cast<int>(stringToHttpMethod("PUT")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpMethod::DELETE), static_cast<int>(stringToHttpMethod("DELETE")));
}

void test_string_to_http_method_case_insensitive() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpMethod::GET), static_cast<int>(stringToHttpMethod("get")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpMethod::POST), static_cast<int>(stringToHttpMethod("post")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HttpMethod::PUT), static_cast<int>(stringToHttpMethod("put")));
}

// Utility function tests
void test_join_strings_empty() {
    std::vector<std::string> strings;
    std::string result = Utils::joinStrings(strings, ", ");
    TEST_ASSERT_EQUAL_STRING("", result.c_str());
}

void test_join_strings_single() {
    std::vector<std::string> strings = {"hello"};
    std::string result = Utils::joinStrings(strings, ", ");
    TEST_ASSERT_EQUAL_STRING("hello", result.c_str());
}

void test_join_strings_multiple() {
    std::vector<std::string> strings = {"hello", "world", "test"};
    std::string result = Utils::joinStrings(strings, ", ");
    TEST_ASSERT_EQUAL_STRING("hello, world, test", result.c_str());
}

void test_join_strings_different_delimiter() {
    std::vector<std::string> strings = {"a", "b", "c"};
    std::string result = Utils::joinStrings(strings, "|");
    TEST_ASSERT_EQUAL_STRING("a|b|c", result.c_str());
}

void test_base64_encode_empty() {
    std::string input = "";
    std::string encoded = Utils::base64Encode(input);
    TEST_ASSERT_EQUAL_STRING("", encoded.c_str());
}

void test_base64_encode_basic() {
    std::string input = "hello";
    std::string encoded = Utils::base64Encode(input);
    TEST_ASSERT_NOT_EQUAL_STRING("", encoded.c_str());
    TEST_ASSERT_NOT_EQUAL_STRING("hello", encoded.c_str());
}

void test_base64_encode_credentials() {
    std::string input = "user:pass";
    std::string encoded = Utils::base64Encode(input);
    TEST_ASSERT_NOT_EQUAL_STRING("", encoded.c_str());
    TEST_ASSERT_NOT_EQUAL_STRING("user:pass", encoded.c_str());
}

void test_generate_boundary() {
    std::string boundary1 = Utils::generateBoundary();
    std::string boundary2 = Utils::generateBoundary();
    
    TEST_ASSERT_NOT_EQUAL_STRING("", boundary1.c_str());
    TEST_ASSERT_NOT_EQUAL_STRING("", boundary2.c_str());
    TEST_ASSERT_NOT_EQUAL_STRING(boundary1.c_str(), boundary2.c_str());
}

void test_parse_status_line_ok() {
    std::string statusLine = "HTTP/1.1 200 OK\r\n";
    HttpResult result;
    
    Utils::parseStatusLine(statusLine, result);
    
    TEST_ASSERT_EQUAL_INT(200, result.statusCode);
    TEST_ASSERT_EQUAL_STRING(" OK\r", result.statusMessage.c_str());
}

void test_parse_status_line_not_found() {
    std::string statusLine = "HTTP/1.1 404 Not Found\r\n";
    HttpResult result;
    
    Utils::parseStatusLine(statusLine, result);
    
    TEST_ASSERT_EQUAL_INT(404, result.statusCode);
    TEST_ASSERT_EQUAL_STRING(" Not Found\r", result.statusMessage.c_str());
}

void test_parse_status_line_server_error() {
    std::string statusLine = "HTTP/1.1 500 Internal Server Error\r\n";
    HttpResult result;
    
    Utils::parseStatusLine(statusLine, result);
    
    TEST_ASSERT_EQUAL_INT(500, result.statusCode);
    TEST_ASSERT_EQUAL_STRING(" Internal Server Error\r", result.statusMessage.c_str());
}

void test_extract_header_value_found() {
    std::unordered_map<std::string, std::string> headers = {
        {"Content-Type", "application/json"},
        {"Content-Length", "123"},
        {"Authorization", "Bearer token123"}
    };
    
    std::string value = Utils::extractHeaderValue(headers, "Content-Type");
    TEST_ASSERT_EQUAL_STRING("application/json", value.c_str());
}

void test_extract_header_value_not_found() {
    std::unordered_map<std::string, std::string> headers = {
        {"Content-Type", "application/json"}
    };
    
    std::string value = Utils::extractHeaderValue(headers, "Authorization");
    TEST_ASSERT_EQUAL_STRING("", value.c_str());
}

void test_extract_content_length_found() {
    std::unordered_map<std::string, std::string> headers = {
        {"Content-Type", "application/json"},
        {"Content-Length", "456"}
    };
    
    size_t length = Utils::extractContentLength(headers);
    TEST_ASSERT_EQUAL_INT(456, length);
}

void test_extract_content_length_not_found() {
    std::unordered_map<std::string, std::string> headers = {
        {"Content-Type", "application/json"}
    };
    
    size_t length = Utils::extractContentLength(headers);
    TEST_ASSERT_EQUAL_INT(0, length);
}

int main() {
    UNITY_BEGIN();
    
    // URL parsing tests
    RUN_TEST(test_extract_scheme_http);
    RUN_TEST(test_extract_scheme_https);
    RUN_TEST(test_extract_scheme_no_scheme);
    RUN_TEST(test_extract_host_basic);
    RUN_TEST(test_extract_host_with_port);
    RUN_TEST(test_extract_host_subdomain);
    RUN_TEST(test_extract_port_default_http);
    RUN_TEST(test_extract_port_default_https);
    RUN_TEST(test_extract_port_custom);
    RUN_TEST(test_extract_path_basic);
    RUN_TEST(test_extract_path_root);
    RUN_TEST(test_extract_path_no_path);
    RUN_TEST(test_extract_path_with_query);
    RUN_TEST(test_extract_base_url);
    RUN_TEST(test_extract_base_url_with_port);
    
    // HTTP Method tests
    RUN_TEST(test_http_method_to_string);
    RUN_TEST(test_string_to_http_method);
    RUN_TEST(test_string_to_http_method_case_insensitive);
    
    // Utility function tests
    RUN_TEST(test_join_strings_empty);
    RUN_TEST(test_join_strings_single);
    RUN_TEST(test_join_strings_multiple);
    RUN_TEST(test_join_strings_different_delimiter);
    RUN_TEST(test_base64_encode_empty);
    RUN_TEST(test_base64_encode_basic);
    RUN_TEST(test_base64_encode_credentials);
    RUN_TEST(test_generate_boundary);
    RUN_TEST(test_parse_status_line_ok);
    RUN_TEST(test_parse_status_line_not_found);
    RUN_TEST(test_parse_status_line_server_error);
    RUN_TEST(test_extract_header_value_found);
    RUN_TEST(test_extract_header_value_not_found);
    RUN_TEST(test_extract_content_length_found);
    RUN_TEST(test_extract_content_length_not_found);
    
    return UNITY_END();
}