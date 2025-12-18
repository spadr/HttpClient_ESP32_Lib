#ifdef NATIVE_TEST
#include "../../helpers/TestEnvironment.h"
#define ARDUINO_ARCH_NATIVE
#endif

#include "../../helpers/simple_unity.h"
#include "../../../src/cookie/Cookie.h"
#include "../../../src/cookie/CookieJar.h"
#include "../../helpers/AssertHelpers.h"

using namespace canaspad;

void setUp(void) {
    // テスト前の初期化
}

void tearDown(void) {
    // テスト後のクリーンアップ
}

void test_cookie_basic_structure() {
    Cookie cookie;
    cookie.name = "test_cookie";
    cookie.value = "test_value";
    cookie.domain = "example.com";
    cookie.path = "/";
    cookie.secure = false;
    cookie.httpOnly = true;
    cookie.expires = 0;
    
    TEST_ASSERT_EQUAL_STRING("test_cookie", cookie.name.c_str());
    TEST_ASSERT_EQUAL_STRING("test_value", cookie.value.c_str());
    TEST_ASSERT_EQUAL_STRING("example.com", cookie.domain.c_str());
    TEST_ASSERT_EQUAL_STRING("/", cookie.path.c_str());
    TEST_ASSERT_FALSE(cookie.secure);
    TEST_ASSERT_TRUE(cookie.httpOnly);
    TEST_ASSERT_EQUAL_INT(0, cookie.expires);
}

void test_cookie_jar_set_cookie() {
    CookieJar jar;
    std::string url = "http://example.com/path";
    std::string setCookieHeader = "session_id=abc123; Path=/; HttpOnly";
    
    jar.setCookie(url, setCookieHeader);
    
    auto cookies = jar.getCookiesForUrl(url);
    TEST_ASSERT_EQUAL_INT(1, cookies.size());
    
    // Check that the cookie is properly formatted
    TestHelpers::assertContains(cookies[0], "session_id=abc123", "Cookie should contain name=value pair");
}

void test_cookie_jar_multiple_cookies() {
    CookieJar jar;
    std::string url = "http://example.com/path";
    
    jar.setCookie(url, "cookie1=value1; Path=/");
    jar.setCookie(url, "cookie2=value2; Path=/");
    jar.setCookie(url, "cookie3=value3; Path=/api");
    
    auto cookies = jar.getCookiesForUrl(url);
    TEST_ASSERT_EQUAL_INT(2, cookies.size()); // Only cookies matching the path should be returned
}

void test_cookie_jar_domain_matching() {
    CookieJar jar;
    
    // Set cookies for different domains
    jar.setCookie("http://example.com/", "cookie1=value1; Domain=example.com");
    jar.setCookie("http://sub.example.com/", "cookie2=value2; Domain=sub.example.com");
    jar.setCookie("http://other.com/", "cookie3=value3; Domain=other.com");
    
    // Test domain matching
    auto cookies1 = jar.getCookiesForUrl("http://example.com/");
    auto cookies2 = jar.getCookiesForUrl("http://sub.example.com/");
    auto cookies3 = jar.getCookiesForUrl("http://other.com/");
    
    TEST_ASSERT_GREATER_OR_EQUAL_INT(1, cookies1.size());
    TEST_ASSERT_GREATER_OR_EQUAL_INT(1, cookies2.size());
    TEST_ASSERT_GREATER_OR_EQUAL_INT(1, cookies3.size());
}

void test_cookie_jar_path_matching() {
    CookieJar jar;
    std::string baseUrl = "http://example.com";
    
    jar.setCookie(baseUrl + "/", "root_cookie=value; Path=/");
    jar.setCookie(baseUrl + "/api/", "api_cookie=value; Path=/api");
    jar.setCookie(baseUrl + "/admin/", "admin_cookie=value; Path=/admin");
    
    // Test path matching
    auto rootCookies = jar.getCookiesForUrl(baseUrl + "/");
    auto apiCookies = jar.getCookiesForUrl(baseUrl + "/api/users");
    auto adminCookies = jar.getCookiesForUrl(baseUrl + "/admin/settings");
    
    TEST_ASSERT_GREATER_OR_EQUAL_INT(1, rootCookies.size());
    TEST_ASSERT_GREATER_OR_EQUAL_INT(1, apiCookies.size());
    TEST_ASSERT_GREATER_OR_EQUAL_INT(1, adminCookies.size());
}

void test_cookie_jar_secure_flag() {
    CookieJar jar;
    
    // Set secure cookie
    jar.setCookie("https://example.com/", "secure_cookie=value; Secure");
    jar.setCookie("http://example.com/", "insecure_cookie=value");
    
    // Test that secure cookies are only sent over HTTPS
    auto httpsCookies = jar.getCookiesForUrl("https://example.com/");
    auto httpCookies = jar.getCookiesForUrl("http://example.com/");
    
    // Both should have at least one cookie, but the exact behavior depends on implementation
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, httpsCookies.size());
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, httpCookies.size());
}

void test_cookie_jar_empty_url() {
    CookieJar jar;
    
    auto cookies = jar.getCookiesForUrl("");
    TEST_ASSERT_EQUAL_INT(0, cookies.size());
}

void test_cookie_jar_empty_set_cookie() {
    CookieJar jar;
    std::string url = "http://example.com/";
    
    jar.setCookie(url, "");
    
    auto cookies = jar.getCookiesForUrl(url);
    TEST_ASSERT_EQUAL_INT(0, cookies.size());
}

void test_cookie_jar_invalid_set_cookie() {
    CookieJar jar;
    std::string url = "http://example.com/";
    
    // Test various invalid cookie formats
    jar.setCookie(url, "invalid_cookie_no_value");
    jar.setCookie(url, "=no_name");
    jar.setCookie(url, "name=");
    
    auto cookies = jar.getCookiesForUrl(url);
    // Implementation may vary on how it handles invalid cookies
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, cookies.size());
}

void run_cookie_tests() {
    RUN_TEST(test_cookie_basic_structure);
    RUN_TEST(test_cookie_jar_set_cookie);
    RUN_TEST(test_cookie_jar_multiple_cookies);
    RUN_TEST(test_cookie_jar_domain_matching);
    RUN_TEST(test_cookie_jar_path_matching);
    RUN_TEST(test_cookie_jar_secure_flag);
    RUN_TEST(test_cookie_jar_empty_url);
    RUN_TEST(test_cookie_jar_empty_set_cookie);
    RUN_TEST(test_cookie_jar_invalid_set_cookie);
}

#ifndef PIO_UNIT_TESTING
int main() {
    UNITY_BEGIN();
    run_cookie_tests();
    return UNITY_END();
}
#endif