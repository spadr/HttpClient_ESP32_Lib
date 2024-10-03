#include "ProxyTest.h"
#include <cstring>

void test_proxy_absolute_form()
{
    canaspad::ClientOptions options;
    options.proxyUrl = "http://proxy.example.com:8080";
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    // プロキシ経由での通信のレスポンスを設定
    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nHello";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("http://example.com/test").setMethod(canaspad::HttpMethod::GET);

    auto result = client.send(request);

    const auto &log = mockClient->getCommunicationLog();

    // GET リクエストの検証
    bool getRequestFound = false;
    std::string expectedGetRequest = "GET http://example.com:80/test HTTP/1.1\r\nHost: example.com:80\r\n\r\n";
    for (const auto &entry : log.getLog())
    {
        if (entry.type == canaspad::CommunicationLog::Entry::Type::Sent &&
            entry.source == canaspad::CommunicationLog::SourceType::Client)
        {
            std::string requestStr(entry.data.begin(), entry.data.end());
            if (requestStr == expectedGetRequest)
            {
                getRequestFound = true;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE(getRequestFound);
}

void test_proxy_authority_form()
{
    canaspad::ClientOptions options;
    options.proxyUrl = "https://proxy.example.com:8080";
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    // プロキシトンネル接続のレスポンスを設定
    const char *connectResponse = "HTTP/1.1 200 Connection established\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(connectResponse, connectResponse + strlen(connectResponse)));

    // プロキシ経由での通信のレスポンスを設定
    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nHello";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("https://example.com/test").setMethod(canaspad::HttpMethod::GET);

    auto result = client.send(request);

    const auto &log = mockClient->getCommunicationLog();

    // CONNECT リクエストの検証
    bool connectRequestFound = false;
    std::string expectedConnectRequest = "CONNECT proxy.example.com:8080 HTTP/1.1\r\nHost: proxy.example.com:8080\r\nConnection: Keep-Alive\r\n\r\n";
    for (const auto &entry : log.getLog())
    {
        if (entry.type == canaspad::CommunicationLog::Entry::Type::Sent &&
            entry.source == canaspad::CommunicationLog::SourceType::Client)
        {
            std::string requestStr(entry.data.begin(), entry.data.end());
            if (requestStr == expectedConnectRequest)
            {
                connectRequestFound = true;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE(connectRequestFound);

    // GET リクエストの検証
    bool getRequestFound = false;
    std::string expectedGetRequest = "GET https://example.com:443/test HTTP/1.1\r\nHost: example.com:443\r\n\r\n";
    for (const auto &entry : log.getLog())
    {
        if (entry.type == canaspad::CommunicationLog::Entry::Type::Sent &&
            entry.source == canaspad::CommunicationLog::SourceType::Client)
        {
            std::string requestStr(entry.data.begin(), entry.data.end());
            if (requestStr == expectedGetRequest)
            {
                getRequestFound = true;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE(getRequestFound);
}

void test_proxy_authentication()
{
    canaspad::ClientOptions options;
    options.proxyUrl = "http://user:password@proxy.example.com:8080";
    options.verifySsl = false;
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    canaspad::Request request;
    request.setUrl("http://example.com/test").setMethod(canaspad::HttpMethod::GET);

    client.send(request);

    const auto &log = mockClient->getCommunicationLog();
    bool proxyAuthFound = false;
    for (const auto &entry : log.getLog())
    {
        if (entry.type == canaspad::CommunicationLog::Entry::Type::Sent)
        {
            std::string requestStr(entry.data.begin(), entry.data.end());
            if (requestStr.find("Proxy-Authorization: Basic dXNlcjpwYXNzd29yZA==") != std::string::npos)
            {
                proxyAuthFound = true;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE(proxyAuthFound);
}

void test_proxy_tunnel_establishment()
{
    canaspad::ClientOptions options;
    options.proxyUrl = "http://proxy.example.com:8080";
    options.verifySsl = false;
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    // CONNECTリクエストに対するレスポンスを設定
    const char *connectResponse = "HTTP/1.1 200 Connection established\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(connectResponse, connectResponse + strlen(connectResponse)));

    // HTTPSリクエストに対するレスポンスを設定
    const char *httpsResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nHello HTTPS!";
    mockClient->injectResponse(std::vector<uint8_t>(httpsResponse, httpsResponse + strlen(httpsResponse)));

    canaspad::Request request;
    request.setUrl("https://example.com/test").setMethod(canaspad::HttpMethod::GET);

    auto result = client.send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Hello HTTPS!", result.value().body.c_str());
}

void test_invalid_proxy_url()
{
    canaspad::ClientOptions options;
    // ポート番号が欠落しているプロキシURL
    options.proxyUrl = "http://proxy.example.com";
    canaspad::HttpClient client1(options, true);
    auto result1 = client1.send(canaspad::Request().setUrl("http://example.com"));
    TEST_ASSERT_TRUE(result1.isError());
    TEST_ASSERT_EQUAL(canaspad::ErrorCode::InvalidURL, result1.error().code);

    // スキームが欠落しているプロキシURL
    options.proxyUrl = "proxy.example.com:8080";
    canaspad::HttpClient client2(options, true);
    auto result2 = client2.send(canaspad::Request().setUrl("http://example.com"));
    TEST_ASSERT_TRUE(result2.isError());
    TEST_ASSERT_EQUAL(canaspad::ErrorCode::InvalidURL, result2.error().code);
}

void test_proxy_connection_failure()
{
    canaspad::ClientOptions options;
    options.proxyUrl = "http://proxy.example.com:8080";
    options.verifySsl = false;
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    // プロキシサーバーへの接続に失敗する設定
    mockClient->setConnectBehavior(canaspad::ConnectBehavior::AlwaysFail);

    canaspad::Request request;
    request.setUrl("http://example.com").setMethod(canaspad::HttpMethod::GET);

    auto result = client.send(request);

    TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(canaspad::ErrorCode::NetworkError, result.error().code);
}

void test_proxy_authentication_error()
{
    canaspad::ClientOptions options;
    options.proxyUrl = "http://user:password@proxy.example.com:8080";
    options.verifySsl = false;
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    // 407 Proxy Authentication Required を返す設定
    const char *response = "HTTP/1.1 407 Proxy Authentication Required\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("http://example.com/test").setMethod(canaspad::HttpMethod::GET);

    auto result = client.send(request);

    TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(canaspad::ErrorCode::ProxyAuthenticationRequired, result.error().code);
}

void run_proxy_tests(void)
{
    RUN_TEST(test_proxy_absolute_form);
    RUN_TEST(test_proxy_authority_form);
    // TODO 未検証のためコメントアウト
    // RUN_TEST(test_proxy_authentication);
    // RUN_TEST(test_proxy_tunnel_establishment);
    // RUN_TEST(test_invalid_proxy_url);
    // RUN_TEST(test_proxy_connection_failure);
    // RUN_TEST(test_proxy_authentication_error);
}