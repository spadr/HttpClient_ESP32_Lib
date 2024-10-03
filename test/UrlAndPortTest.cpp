#include "UrlAndPortTest.h"

void test_http_default_port(void)
{
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("http://example.com").setMethod(canaspad::HttpMethod::GET);

    auto result = client.send(request);
    TEST_ASSERT_TRUE(result.isSuccess());

    // 接続先ポートの検証
    const auto &log = mockClient->getCommunicationLog();
    for (const auto &entry : log.getLog())
    {
        if (entry.type == canaspad::CommunicationLog::Entry::Type::Sent &&
            entry.source == canaspad::CommunicationLog::SourceType::Client)
        {
            std::string requestStr(entry.data.begin(), entry.data.end());
            // リクエストラインからポート番号を抽出
            std::string portStr = requestStr.substr(requestStr.find("example.com:") + 12, 2);
            // デフォルトポート 80 で接続されていることを確認
            TEST_ASSERT_EQUAL_STRING("80", portStr.c_str());
            break;
        }
    }
}

void test_https_default_port(void)
{
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("https://example.com").setMethod(canaspad::HttpMethod::GET);

    auto result = client.send(request);
    TEST_ASSERT_TRUE(result.isSuccess());

    // 接続先ポートの検証
    const auto &log = mockClient->getCommunicationLog();
    for (const auto &entry : log.getLog())
    {
        if (entry.type == canaspad::CommunicationLog::Entry::Type::Sent &&
            entry.source == canaspad::CommunicationLog::SourceType::Client)
        {
            std::string requestStr(entry.data.begin(), entry.data.end());
            // リクエストラインからポート番号を抽出
            std::string portStr = requestStr.substr(requestStr.find("example.com:") + 12, 3);
            // デフォルトポート 443 で接続されていることを確認
            TEST_ASSERT_EQUAL_STRING("443", portStr.c_str());
            break;
        }
    }
}

void test_http_explicit_port(void)
{
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("http://example.com:8080").setMethod(canaspad::HttpMethod::GET);

    auto result = client.send(request);
    TEST_ASSERT_TRUE(result.isSuccess());

    // 接続先ポートの検証
    const auto &log = mockClient->getCommunicationLog();
    for (const auto &entry : log.getLog())
    {
        if (entry.type == canaspad::CommunicationLog::Entry::Type::Sent &&
            entry.source == canaspad::CommunicationLog::SourceType::Client)
        {
            std::string requestStr(entry.data.begin(), entry.data.end());
            // リクエストラインからポート番号を抽出
            std::string portStr = requestStr.substr(requestStr.find("example.com:") + 12, 4);
            // 指定したポート 8080 で接続されていることを確認
            TEST_ASSERT_EQUAL_STRING("8080", portStr.c_str());
            break;
        }
    }
}

void test_https_explicit_port(void)
{
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("https://example.com:8080").setMethod(canaspad::HttpMethod::GET);

    auto result = client.send(request);
    TEST_ASSERT_TRUE(result.isSuccess());

    // 接続先ポートの検証
    const auto &log = mockClient->getCommunicationLog();
    for (const auto &entry : log.getLog())
    {
        if (entry.type == canaspad::CommunicationLog::Entry::Type::Sent &&
            entry.source == canaspad::CommunicationLog::SourceType::Client)
        {
            std::string requestStr(entry.data.begin(), entry.data.end());
            // リクエストラインからポート番号を抽出
            std::string portStr = requestStr.substr(requestStr.find("example.com:") + 12, 4);
            // 指定したポート 8080 で接続されていることを確認
            TEST_ASSERT_EQUAL_STRING("8080", portStr.c_str());
            break;
        }
    }
}

void run_url_and_port_tests(void)
{
    RUN_TEST(test_http_default_port);
    RUN_TEST(test_https_default_port);
    RUN_TEST(test_http_explicit_port);
    RUN_TEST(test_https_explicit_port);
}