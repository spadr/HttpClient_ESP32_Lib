#include "CookieTest.h"
#include <ctime>

void test_cookie_jar_set_and_get_cookies()
{
    canaspad::CookieJar cookieJar;

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
    canaspad::CookieJar cookieJar;

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
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    client.enableCookies(true); // クッキー処理を有効にする

    // クッキーを設定するレスポンス
    const char *response1 =
        "HTTP/1.1 200 OK\r\n"
        "Set-Cookie: session_id=12345; Domain=example.com; Path=/; HttpOnly\r\n"
        "Content-Length: 0\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response1, response1 + strlen(response1)));

    // 最初のGETリクエスト
    canaspad::Request request1;
    request1.setUrl("https://example.com/").setMethod(canaspad::Request::Method::GET);
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
    canaspad::Request request2;
    request2.setUrl("https://example.com/resource").setMethod(canaspad::Request::Method::GET);
    auto result2 = client.send(request2);

    // レスポンスボディを確認
    TEST_ASSERT_TRUE(result2.isSuccess());
    TEST_ASSERT_EQUAL_STRING("Cookie received!", result2.value().body.c_str());

    // 送信されたリクエストヘッダーにクッキーが含まれていることを確認
    const auto &log = mockClient->getCommunicationLog();
    bool cookieFound = false;
    for (const auto &entry : log.getLog())
    {
        if (entry.type == canaspad::CommunicationLog::Entry::Type::Sent)
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

void run_cookie_tests(void)
{
    RUN_TEST(test_cookie_jar_set_and_get_cookies);
    RUN_TEST(test_cookie_jar_expired_cookie);
    RUN_TEST(test_http_client_cookie_handling);
}