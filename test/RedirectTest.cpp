#include "RedirectTest.h"
#include <cstring>

void test_http_client_redirect_handling()
{
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    // リダイレクトレスポンス (302 Found)
    const char *response1 =
        "HTTP/1.1 302 Found\r\n"
        "Location: https://example.com/redirected\r\n"
        "Content-Length: 0\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response1, response1 + strlen(response1)));

    // 最終的なレスポンス
    const char *response2 =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 18\r\n\r\n"
        "Redirected page!";
    mockClient->injectResponse(std::vector<uint8_t>(response2, response2 + strlen(response2)));

    // 最初のGETリクエスト
    canaspad::Request request;
    request.setUrl("https://example.com").setMethod(canaspad::Request::Method::GET);
    auto result = client.send(request);

    // リダイレクトを追跡し、最終的なレスポンスが取得できていることを確認
    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Redirected page!", result.value().body.c_str());
}

void test_http_client_disable_redirect_following()
{
    canaspad::ClientOptions options;
    options.verifySsl = false;
    options.followRedirects = false; // リダイレクト追跡を無効にする
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    // リダイレクトレスポンス (302 Found)
    const char *response =
        "HTTP/1.1 302 Found\r\n"
        "Location: https://example.com/redirected\r\n"
        "Content-Length: 0\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    // 最初のGETリクエスト
    canaspad::Request request;
    request.setUrl("https://example.com").setMethod(canaspad::Request::Method::GET);
    auto result = client.send(request);

    // リダイレクトを追跡せず、302 Found レスポンスが取得できていることを確認
    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(302, result.value().statusCode);
}

void test_http_client_too_many_redirects()
{
    canaspad::ClientOptions options;
    options.verifySsl = false;
    options.maxRedirects = 3; // リダイレクト最大回数を3に設定
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    // リダイレクトレスポンス (302 Found) を4回注入
    const char *response =
        "HTTP/1.1 302 Found\r\n"
        "Location: https://example.com/redirected\r\n"
        "Content-Length: 0\r\n\r\n";
    for (int i = 0; i < 4; ++i)
    {
        mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));
    }

    // 最初のGETリクエスト
    canaspad::Request request;
    request.setUrl("https://example.com").setMethod(canaspad::Request::Method::GET);
    auto result = client.send(request);

    // リダイレクト回数が最大を超えたため、エラーが発生することを確認
    TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(canaspad::ErrorCode::TooManyRedirects, result.error().code);
}

void run_redirect_tests(void)
{
    RUN_TEST(test_http_client_redirect_handling);
    RUN_TEST(test_http_client_disable_redirect_following);
    RUN_TEST(test_http_client_too_many_redirects);
}