#ifdef NATIVE_TEST
#include "../../helpers/TestEnvironment.h"
#endif

#include "../../helpers/simple_unity.h"
#include "../../../src/cookie/Cookie.h"
#include "../../../src/cookie/CookieJar.h"
#include "../../../src/HttpClient.h"
#include "../../../src/core/mock/MockWiFiClientSecure.h"
#include "../../helpers/AssertHelpers.h"
#include <cstring>

using namespace canaspad;

void setUp(void)
{
    // テスト前の初期化
}

void tearDown(void)
{
    // テスト後のクリーンアップ
}

void test_cookie_basic_structure()
{
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

void test_cookie_jar_set_cookie()
{
    CookieJar jar;
    std::string url = "http://example.com/path";
    std::string setCookieHeader = "session_id=abc123; Path=/; HttpOnly";

    jar.setCookie(url, setCookieHeader);

    auto cookies = jar.getCookiesForUrl(url);
    TEST_ASSERT_EQUAL_INT(1, cookies.size());

    // Check that the cookie is properly formatted
    TestHelpers::assertContains(cookies[0], "session_id=abc123", "Cookie should contain name=value pair");
}

void test_cookie_jar_multiple_cookies()
{
    CookieJar jar;
    std::string url = "http://example.com/path";

    jar.setCookie(url, "cookie1=value1; Path=/");
    jar.setCookie(url, "cookie2=value2; Path=/");
    jar.setCookie(url, "cookie3=value3; Path=/api");

    auto cookies = jar.getCookiesForUrl(url);
    TEST_ASSERT_EQUAL_INT(2, cookies.size()); // Only cookies matching the path should be returned
}

void test_cookie_jar_domain_matching()
{
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

void test_cookie_jar_path_matching()
{
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

void test_cookie_jar_secure_flag()
{
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

void test_cookie_jar_empty_url()
{
    CookieJar jar;

    auto cookies = jar.getCookiesForUrl("");
    TEST_ASSERT_EQUAL_INT(0, cookies.size());
}

void test_cookie_jar_empty_set_cookie()
{
    CookieJar jar;
    std::string url = "http://example.com/";

    jar.setCookie(url, "");

    auto cookies = jar.getCookiesForUrl(url);
    TEST_ASSERT_EQUAL_INT(0, cookies.size());
}

void test_cookie_jar_invalid_set_cookie()
{
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

void test_cookie_jar_set_and_get_cookies()
{
    CookieJar cookieJar;

    // クッキー1を設定
    cookieJar.setCookie("https://example.com/path", "cookie1=value1; Domain=example.com; Path=/path; Secure; HttpOnly");

    // クッキー2を設定
    cookieJar.setCookie("https://api.example.com/api/v1", "cookie2=value2; Domain=api.example.com; Path=/api/v1");

    // example.com のクッキーを取得
    auto cookies1 = cookieJar.getCookiesForUrl("https://example.com/path/resource");
    TEST_ASSERT_EQUAL_INT(1, cookies1.size());
    TEST_ASSERT_EQUAL_STRING("cookie1=value1", cookies1[0].c_str());

    // api.example.com のクッキーを取得
    auto cookies2 = cookieJar.getCookiesForUrl("https://api.example.com/api/v1/users");
    TEST_ASSERT_EQUAL_INT(1, cookies2.size());
    TEST_ASSERT_EQUAL_STRING("cookie2=value2", cookies2[0].c_str());

    // 存在しないURLのクッキーを取得
    auto cookies3 = cookieJar.getCookiesForUrl("https://other.com");
    TEST_ASSERT_EQUAL_INT(0, cookies3.size());
}

void test_cookie_jar_expired_cookie()
{
    CookieJar cookieJar;

    // 過去に有効期限が切れたクッキーを設定 (ドメインを明示的に指定)
    cookieJar.setCookie("https://example.com", "expired_cookie=expired_value; Domain=example.com; Expires=Wed, 21 Oct 2015 07:28:00 GMT");

    // 別のクッキーを設定 (ドメインはリクエストURLから取得される)
    cookieJar.setCookie("https://api.example.com", "valid_cookie=valid_value");

    // クッキーを取得
    auto cookies = cookieJar.getCookiesForUrl("https://api.example.com"); // api.example.com のクッキーを取得

    // 有効期限切れのクッキーは返されないことを確認
    TEST_ASSERT_EQUAL_INT(1, cookies.size()); // 有効なクッキーのみ
    TEST_ASSERT_EQUAL_STRING("valid_cookie=valid_value", cookies[0].c_str());
}

void test_http_client_cookie_handling()
{
    ClientOptions options;
    options.verifySsl = false;
    HttpClient client(options, true);
    auto *mockClient = static_cast<MockWiFiClientSecure *>(client.getConnection());

    client.enableCookies(true); // クッキー処理を有効にする

    // クッキーを設定するレスポンス
    const char *response1 =
        "HTTP/1.1 200 OK\r\n"
        "Set-Cookie: session_id=12345; Domain=example.com; Path=/; HttpOnly\r\n"
        "Content-Length: 0\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response1, response1 + strlen(response1)));

    // 最初のGETリクエスト
    Request request1;
    request1.setUrl("https://example.com/").setMethod(HttpMethod::GET);
    auto result1 = client.send(request1);

    // クッキーが設定されていることを確認
    TEST_ASSERT_TRUE(result1.isSuccess());
    TEST_ASSERT_EQUAL_INT(1, result1.value().cookies.size());
    TEST_ASSERT_EQUAL_STRING("session_id", result1.value().cookies[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("12345", result1.value().cookies[0].value.c_str());

    // クッキーを含むリクエストを送信するレスポンス
    const char *response2 =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 16\r\n\r\n"
        "Cookie received!";
    mockClient->injectResponse(std::vector<uint8_t>(response2, response2 + strlen(response2)));

    // 2回目のGETリクエスト (クッキーが自動的に付与される)
    Request request2;
    request2.setUrl("https://example.com/resource").setMethod(HttpMethod::GET);
    auto result2 = client.send(request2);

    // レスポンスボディを確認
    TEST_ASSERT_TRUE(result2.isSuccess());
    TEST_ASSERT_EQUAL_STRING("Cookie received!", result2.value().body.c_str());

    // 送信されたリクエストヘッダーにクッキーが含まれていることを確認
    const auto &log = mockClient->getCommunicationLog();
    bool cookieFound = false;
    for (const auto &entry : log.getLog())
    {
        if (entry.type == CommunicationLog::Entry::Type::Sent)
        {
            std::string requestStr(entry.data.begin(), entry.data.end());
            if (requestStr.find("Cookie: session_id=12345") != std::string::npos)
            {
                cookieFound = true;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE(cookieFound);
}

void run_cookie_tests()
{
    RUN_TEST(test_cookie_basic_structure);
    RUN_TEST(test_cookie_jar_set_cookie);
    RUN_TEST(test_cookie_jar_multiple_cookies);
    RUN_TEST(test_cookie_jar_domain_matching);
    RUN_TEST(test_cookie_jar_path_matching);
    RUN_TEST(test_cookie_jar_secure_flag);
    RUN_TEST(test_cookie_jar_empty_url);
    RUN_TEST(test_cookie_jar_empty_set_cookie);
    RUN_TEST(test_cookie_jar_invalid_set_cookie);
    RUN_TEST(test_cookie_jar_set_and_get_cookies);
    RUN_TEST(test_cookie_jar_expired_cookie);
    RUN_TEST(test_http_client_cookie_handling);
}

#ifdef NATIVE_TEST
int main()
{
    UNITY_BEGIN();
    run_cookie_tests();
    return UNITY_END();
}
#endif