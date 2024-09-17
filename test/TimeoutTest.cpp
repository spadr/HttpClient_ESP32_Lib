#include "TimeoutTest.h"
#include <cstring>
#include <chrono>
#include <thread>

void test_connection_timeout()
{
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    // 常に接続に失敗する設定
    mockClient->setConnectBehavior(canaspad::ConnectBehavior::AlwaysFail);

    canaspad::Request request;
    request.setUrl("https://example.com").setMethod(canaspad::Request::Method::GET);

    auto result = client.send(request);

    // タイムアウトエラーが発生することを確認
    TEST_ASSERT_TRUE(result.isError());
}

void test_read_timeout()
{
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());
    client.setReadTimeout(std::chrono::milliseconds(1000)); // 1秒に設定

    // 接続は成功するが、データを送信しない設定
    mockClient->setConnectBehavior(canaspad::ConnectBehavior::AlwaysSuccess);
    mockClient->setReadBehavior(canaspad::ReadBehavior::Timeout, std::chrono::milliseconds(500));

    canaspad::Request request;
    request.setUrl("https://example.com").setMethod(canaspad::Request::Method::GET);

    auto result = client.send(request);

    // タイムアウトエラーが発生することを確認
    TEST_ASSERT_TRUE(result.isError());
}

void test_write_timeout()
{
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());
    client.setWriteTimeout(std::chrono::milliseconds(1000)); // 1秒に設定

    // 書き込みタイムアウトを発生させる
    mockClient->setWriteBehavior(canaspad::WriteBehavior::Timeout, std::chrono::milliseconds(500));

    canaspad::Request request;
    request.setUrl("https://example.com").setMethod(canaspad::Request::Method::POST).setBody("This is a test body.");

    auto result = client.send(request);

    // タイムアウトエラーが発生することを確認
    TEST_ASSERT_TRUE(result.isError());
}

void run_timeout_tests(void)
{
    RUN_TEST(test_connection_timeout);
    RUN_TEST(test_read_timeout);
    RUN_TEST(test_write_timeout);
}