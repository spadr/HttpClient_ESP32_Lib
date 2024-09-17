#include "RetryTest.h"
#include <cstring>
#include <chrono>
#include <thread>

void test_http_client_retry_on_network_error()
{
    canaspad::ClientOptions options;
    options.verifySsl = false;
    options.maxRetries = 3;
    options.retryDelay = std::chrono::milliseconds(100); // 100ミリ秒の遅延
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    // 最初の2回は接続に失敗し、3回目で成功する設定
    mockClient->setConnectBehavior(canaspad::ConnectBehavior::FailNTimesThenSuccess, 2);

    // 成功時のレスポンス
    const char *successResponse =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 12\r\n\r\n"
        "Success!";
    mockClient->injectResponse(std::vector<uint8_t>(successResponse, successResponse + strlen(successResponse)));

    canaspad::Request request;
    request.setUrl("https://example.com").setMethod(canaspad::Request::Method::GET);

    auto start = std::chrono::high_resolution_clock::now();
    auto result = client.send(request);
    auto end = std::chrono::high_resolution_clock::now();

    // リクエストが成功したことを確認
    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Success!", result.value().body.c_str());

    // リトライによる遅延が発生していることを確認
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    TEST_ASSERT_TRUE(duration.count() >= 180 && duration.count() <= 220); // 2回のリトライ x 100ミリ秒
}

void test_http_client_retry_max_retries_exceeded()
{
    canaspad::ClientOptions options;
    options.verifySsl = false;
    options.maxRetries = 2;
    options.retryDelay = std::chrono::milliseconds(50); // 50ミリ秒の遅延
    canaspad::HttpClient client(options, true);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    // 常に接続に失敗する設定
    mockClient->setConnectBehavior(canaspad::ConnectBehavior::AlwaysFail);

    canaspad::Request request;
    request.setUrl("https://example.com").setMethod(canaspad::Request::Method::GET);

    auto start = std::chrono::high_resolution_clock::now();
    auto result = client.send(request);
    auto end = std::chrono::high_resolution_clock::now();

    // リクエストが失敗したことを確認
    TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(canaspad::ErrorCode::NetworkError, result.error().code);

    // 最大リトライ回数を超えたため、遅延は2回分のみ発生
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    TEST_ASSERT_TRUE(duration.count() >= 80 && duration.count() <= 120); // 2回のリトライ x 50ミリ秒
}

void run_retry_tests(void)
{
    RUN_TEST(test_http_client_retry_on_network_error);
    RUN_TEST(test_http_client_retry_max_retries_exceeded);
}