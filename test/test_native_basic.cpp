#ifdef NATIVE_TEST
#include "helpers/TestEnvironment.h"
#define ARDUINO_ARCH_NATIVE
#endif

#include "helpers/simple_unity.h"
#include "../src/utils/Utils.h"
#include "../src/utils/HttpMethod.h"

using namespace canaspad;

void setUp(void) {
    // テスト前の初期化
}

void tearDown(void) {
    // テスト後のクリーンアップ
}

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

void test_http_method_to_string() {
    TEST_ASSERT_EQUAL_STRING("GET", httpMethodToString(HttpMethod::GET).c_str());
    TEST_ASSERT_EQUAL_STRING("POST", httpMethodToString(HttpMethod::POST).c_str());
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_extract_scheme_http);
    RUN_TEST(test_extract_scheme_https);
    RUN_TEST(test_http_method_to_string);
    
    return UNITY_END();
}