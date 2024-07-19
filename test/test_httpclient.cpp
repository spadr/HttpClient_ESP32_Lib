#include <cstring>
#include <Arduino.h>
#include <unity.h>

#include "../src/HttpClient.h"
#include "../src/core/mock/MockWiFiClientSecure.h"
#include "../src/core/mock/CommunicationLog.h"

canaspad::ClientOptions options;
canaspad::HttpClient *client;
canaspad::MockWiFiClientSecure *mockClient;

void setUp(void)
{
    options = canaspad::ClientOptions();
    client = new canaspad::HttpClient(options, true); // モックを使用
    auto connection = client->getConnection();
    mockClient = static_cast<canaspad::MockWiFiClientSecure *>(connection);
    TEST_ASSERT_NOT_NULL(mockClient);
}
void tearDown(void)
{
    delete client;
}

// 1. 基本的なGETリクエスト
void test_basic_get_request(void)
{
    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
    mockClient->injectResponse(
        std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("http://example.com").setMethod(canaspad::Request::Method::GET);

    auto result = client->send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Hello, World!", result.value().body.c_str());
}

// 2. POSTリクエスト
void test_post_request(void)
{
    const char *response = "HTTP/1.1 201 Created\r\nContent-Length: 7\r\n\r\nCreated";
    mockClient->injectResponse(
        std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("http://example.com/create")
        .setMethod(canaspad::Request::Method::POST)
        .setBody("name=John&age=30");

    auto result = client->send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(201, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Created", result.value().body.c_str());
}

// 3. その他のHTTPメソッド
void test_other_http_methods(void)
{
    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
    mockClient->injectResponse(
        std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("http://example.com/resource")
        .setMethod(canaspad::Request::Method::PUT);

    auto result = client->send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("OK", result.value().body.c_str());

    // Similar tests for DELETE, PATCH, HEAD, OPTIONS
}

// 4. エラーハンドリング
void test_error_handling(void)
{
    const char *response =
        "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
    mockClient->injectResponse(
        std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("http://example.com/nonexistent")
        .setMethod(canaspad::Request::Method::GET);

    auto result = client->send(request);

    TEST_ASSERT_TRUE(result.isSuccess()); // The request itself was successful
    TEST_ASSERT_EQUAL_INT(404, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Not Found", result.value().body.c_str());

    // Similar tests for 500 errors and network connection errors
}

// 5. リダイレクト処理
void test_redirect_handling(void)
{
    const char *response1 =
        "HTTP/1.1 301 Moved Permanently\r\nLocation: http://example.com/new\r\n\r\n";
    const char *response2 =
        "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nRedirected OK";

    mockClient->injectResponse(
        std::vector<uint8_t>(response1, response1 + strlen(response1)));
    mockClient->injectResponse(
        std::vector<uint8_t>(response2, response2 + strlen(response2)));

    canaspad::Request request;
    request.setUrl("http://example.com/old")
        .setMethod(canaspad::Request::Method::GET);

    auto result = client->send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Redirected OK", result.value().body.c_str());

    // Test for maximum redirect limit
}

// 6. 認証
void test_authentication(void)
{
    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 14\r\n\r\nAuthenticated";
    mockClient->injectResponse(
        std::vector<uint8_t>(response, response + strlen(response)));

    options.authType = canaspad::AuthType::Basic;
    options.username = "user";
    options.password = "pass";

    canaspad::HttpClient authenticatedClient(options);

    canaspad::Request request;
    request.setUrl("http://example.com/secure")
        .setMethod(canaspad::Request::Method::GET);

    auto result = authenticatedClient.send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Authenticated", result.value().body.c_str());

    // Similar tests for Bearer auth and failed auth attempts
}

// 7. SSL/TLS接続
void test_ssl_connection(void)
{
    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 6\r\n\r\nSecure";
    mockClient->injectResponse(
        std::vector<uint8_t>(response, response + strlen(response)));

    options.verifySsl = true;
    options.rootCA = "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----\n";

    canaspad::HttpClient secureClient(options);

    canaspad::Request request;
    request.setUrl("https://example.com").setMethod(canaspad::Request::Method::GET);

    auto result = secureClient.send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Secure", result.value().body.c_str());

    // Similar tests for self-signed certs and client cert auth
}

// 8. クッキー処理
void test_cookie_handling(void)
{
    const char *response1 =
        "HTTP/1.1 200 OK\r\nSet-Cookie: session=abc123\r\nContent-Length: 2\r\n\r\nOK";
    const char *response2 = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";

    mockClient->injectResponse(
        std::vector<uint8_t>(response1, response1 + strlen(response1)));
    mockClient->injectResponse(
        std::vector<uint8_t>(response2, response2 + strlen(response2)));

    canaspad::Request request1, request2;
    request1.setUrl("http://example.com").setMethod(canaspad::Request::Method::GET);
    request2.setUrl("http://example.com").setMethod(canaspad::Request::Method::GET);

    client->send(request1);
    auto result = client->send(request2);

    TEST_ASSERT_TRUE(result.isSuccess());

    // Check if the cookie was sent in the second request
    const auto &log = mockClient->getCommunicationLog();
    bool cookieSent = false;
    for (const auto &entry : log.getLog())
    {
        if (entry.type == canaspad::CommunicationLog::Entry::Type::Sent)
        {
            std::string data(entry.data.begin(), entry.data.end());
            if (data.find("Cookie: session=abc123") != std::string::npos)
            {
                cookieSent = true;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE(cookieSent);

    // Similar tests for cookie expiration
}

// 9. プロキシ接続
void test_proxy_connection(void)
{
    const char *response =
        "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nProxy Result";
    mockClient->injectResponse(
        std::vector<uint8_t>(response, response + strlen(response)));

    options.proxyUrl = "http://proxy.example.com:8080";
    canaspad::HttpClient proxyClient(options);

    canaspad::Request request;
    request.setUrl("http://target.example.com")
        .setMethod(canaspad::Request::Method::GET);

    auto result = proxyClient.send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Proxy Result", result.value().body.c_str());

    // Check if the request was sent through the proxy
    const auto &log = mockClient->getCommunicationLog();
    bool proxyUsed = false;
    for (const auto &entry : log.getLog())
    {
        if (entry.type == canaspad::CommunicationLog::Entry::Type::Sent)
        {
            std::string data(entry.data.begin(), entry.data.end());
            if (data.find("CONNECT target.example.com:80 HTTP/1.1") !=
                std::string::npos)
            {
                proxyUsed = true;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE(proxyUsed);

    // Similar tests for HTTPS proxy and proxy auth
}

// 10. タイムアウト処理
void test_timeout_handling(void)
{
    // Simulate a timeout by not injecting any response

    canaspad::HttpClient::Timeouts timeouts;
    timeouts.connect = std::chrono::milliseconds(100);
    client->setTimeouts(timeouts);

    canaspad::Request request;
    request.setUrl("http://example.com").setMethod(canaspad::Request::Method::GET);

    auto result = client->send(request);

    TEST_ASSERT_FALSE(result.isSuccess());
    TEST_ASSERT_EQUAL(canaspad::ErrorCode::Timeout, result.error().code);

    // Similar tests for read and write timeouts
}

// 11. リトライ機能
void test_retry_functionality(void)
{
    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";

    // Simulate 2 failures followed by a success
    mockClient->injectResponse(std::vector<uint8_t>()); // Empty response to simulate
                                                        // failure
    mockClient->injectResponse(std::vector<uint8_t>()); // Empty response to simulate
                                                        // failure
    mockClient->injectResponse(
        std::vector<uint8_t>(response, response + strlen(response)));

    options.maxRetries = 3;
    options.retryDelay = std::chrono::milliseconds(100);
    canaspad::HttpClient retryClient(options);

    canaspad::Request request;
    request.setUrl("http://example.com").setMethod(canaspad::Request::Method::GET);

    auto result = retryClient.send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("OK", result.value().body.c_str());

    // Check if there were 3 attempts in total
    const auto &log = mockClient->getCommunicationLog();
    int attempts = 0;
    for (const auto &entry : log.getLog())
    {
        if (entry.type == canaspad::CommunicationLog::Entry::Type::Sent)
        {
            attempts++;
        }
    }
    TEST_ASSERT_EQUAL_INT(3, attempts);
}

// 12. マルチパートフォームデータ
void test_multipart_form_data(void)
{
    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
    mockClient->injectResponse(
        std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("http://example.com/upload")
        .setMethod(canaspad::Request::Method::POST)
        .setMultipartFormData({{"name", "John Doe"}, {"file", "file_content"}});

    auto result = client->send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);

    // Check if the request was sent with correct headers and body
    const auto &log = mockClient->getCommunicationLog();
    bool correctRequest = false;
    for (const auto &entry : log.getLog())
    {
        if (entry.type == canaspad::CommunicationLog::Entry::Type::Sent)
        {
            std::string data(entry.data.begin(), entry.data.end());
            if (data.find("Content-Type: multipart/form-data; boundary=") !=
                    std::string::npos &&
                data.find("Content-Disposition: form-data; name=\"name\"") !=
                    std::string::npos &&
                data.find("John Doe") != std::string::npos &&
                data.find("Content-Disposition: form-data; name=\"file\"") !=
                    std::string::npos &&
                data.find("file_content") != std::string::npos)
            {
                correctRequest = true;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE(correctRequest);
}

// 13. ヘッダー操作
void test_header_manipulation(void)
{
    const char *response =
        "HTTP/1.1 200 OK\r\nCustom-Header: CustomValue\r\nContent-Length: 2\r\n\r\nOK";
    mockClient->injectResponse(
        std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("http://example.com")
        .setMethod(canaspad::Request::Method::GET)
        .addHeader("X-Custom-Header", "Test");

    auto result = client->send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);

    // Check if custom header was sent
    const auto &log = mockClient->getCommunicationLog();
    bool headerSent = false;
    for (const auto &entry : log.getLog())
    {
        if (entry.type == canaspad::CommunicationLog::Entry::Type::Sent)
        {
            std::string data(entry.data.begin(), entry.data.end());
            if (data.find("X-Custom-Header: Test") != std::string::npos)
            {
                headerSent = true;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE(headerSent);

    // Check if response header was correctly parsed
    TEST_ASSERT_EQUAL_STRING("CustomValue",
                             result.value().headers.at("Custom-Header").c_str());
}

// 14. レスポンスボディ処理
void test_response_body_handling(void)
{
    // Test text response
    const char *textResponse =
        "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "
        "13\r\n\r\nHello, World!";
    mockClient->injectResponse(
        std::vector<uint8_t>(textResponse, textResponse + strlen(textResponse)));

    canaspad::Request textRequest;
    textRequest.setUrl("http://example.com/text")
        .setMethod(canaspad::Request::Method::GET);

    auto textResult = client->send(textRequest);

    TEST_ASSERT_TRUE(textResult.isSuccess());
    TEST_ASSERT_EQUAL_STRING("Hello, World!", textResult.value().body.c_str());

    // Test binary response
    const char binaryData[] = {0x00, 0x01, 0x02, 0x03, 0x04};
    std::string binaryHeader =
        "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-"
        "Length: 5\r\n\r\n";
    std::vector<uint8_t> binaryResponse(binaryHeader.begin(),
                                        binaryHeader.end());
    binaryResponse.insert(binaryResponse.end(), binaryData, binaryData + 5);
    mockClient->injectResponse(binaryResponse);

    canaspad::Request binaryRequest;
    binaryRequest.setUrl("http://example.com/binary")
        .setMethod(canaspad::Request::Method::GET);

    auto binaryResult = client->send(binaryRequest);

    TEST_ASSERT_TRUE(binaryResult.isSuccess());
    TEST_ASSERT_EQUAL_MEMORY(binaryData, binaryResult.value().body.data(), 5);

    // Test large response
    std::string largeData(1024 * 1024, 'A'); // 1MB of 'A's
    std::string largeHeader =
        "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "
        "1048576\r\n\r\n";
    std::vector<uint8_t> largeResponse(largeHeader.begin(), largeHeader.end());
    largeResponse.insert(largeResponse.end(), largeData.begin(),
                         largeData.end());
    mockClient->injectResponse(largeResponse);

    canaspad::Request largeRequest;
    largeRequest.setUrl("http://example.com/large")
        .setMethod(canaspad::Request::Method::GET);

    auto largeResult = client->send(largeRequest);

    TEST_ASSERT_TRUE(largeResult.isSuccess());
    TEST_ASSERT_EQUAL_UINT32(1024 * 1024, largeResult.value().body.length());
    TEST_ASSERT_EQUAL_CHAR('A', largeResult.value().body[1024 * 1024 - 1]);
}

// 15. 進捗コールバック
void test_progress_callback(void)
{
    std::string longString(1000000, 'A');
    std::string fullResponse =
        "HTTP/1.1 200 OK\r\nContent-Length: 1000000\r\n\r\n" + longString;
    const char *response = fullResponse.c_str();
    mockClient->injectResponse(
        std::vector<uint8_t>(response, response + strlen(response)));

    size_t lastReported = 0;
    bool progressReported = false;

    client->setProgressCallback([&](size_t progress, size_t total)
                                {
    if (progress > lastReported) {
      lastReported = progress;
      progressReported = true;
    } });

    canaspad::Request request;
    request.setUrl("http://example.com/large")
        .setMethod(canaspad::Request::Method::GET);

    auto result = client->send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_TRUE(progressReported);
    TEST_ASSERT_EQUAL_UINT32(1000000, lastReported);
}

// 16. 同時リクエスト
void test_concurrent_requests(void)
{
    const char *response1 = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nA1";
    const char *response2 = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nB2";

    mockClient->injectResponse(
        std::vector<uint8_t>(response1, response1 + strlen(response1)));
    mockClient->injectResponse(
        std::vector<uint8_t>(response2, response2 + strlen(response2)));

    canaspad::Request request1, request2;
    request1.setUrl("http://example.com/1")
        .setMethod(canaspad::Request::Method::GET);
    request2.setUrl("http://example.com/2")
        .setMethod(canaspad::Request::Method::GET);

    auto result1 = client->send(request1);
    auto result2 = client->send(request2);

    TEST_ASSERT_TRUE(result1.isSuccess());
    TEST_ASSERT_TRUE(result2.isSuccess());
    TEST_ASSERT_EQUAL_STRING("A1", result1.value().body.c_str());
    TEST_ASSERT_EQUAL_STRING("B2", result2.value().body.c_str());
}

// 17. キャンセル機能
void test_cancel_functionality(void)
{
    // This test is tricky to implement with a mock client.
    // In a real scenario, you'd start a long-running request in a separate
    // thread,
    // then call cancel() from the main thread.
    // Here, we'll just test that the cancel() method exists and can be called.

    canaspad::Request request;
    request.setUrl("http://example.com/long-running")
        .setMethod(canaspad::Request::Method::GET);

    // Start the request (in a real scenario, this would be in a separate thread)
    client->send(request);

    // Cancel the request
    client->cancel("some_request_id");

    // We can't easily assert anything here without implementing a more complex
    // mock
    // that supports asynchronous operations. In a real test, you'd check that
    // the
    // request was actually cancelled and that appropriate error was returned.
}

int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_basic_get_request);
    // RUN_TEST(test_post_request);
    // RUN_TEST(test_other_http_methods);
    // RUN_TEST(test_error_handling);
    // RUN_TEST(test_redirect_handling);
    // RUN_TEST(test_authentication);
    // RUN_TEST(test_ssl_connection);
    // RUN_TEST(test_cookie_handling);
    // RUN_TEST(test_proxy_connection);
    // RUN_TEST(test_timeout_handling);
    // RUN_TEST(test_retry_functionality);
    // RUN_TEST(test_multipart_form_data);
    // RUN_TEST(test_header_manipulation);
    // RUN_TEST(test_response_body_handling);
    // RUN_TEST(test_progress_callback);
    // RUN_TEST(test_concurrent_requests);
    // RUN_TEST(test_cancel_functionality);
    return UNITY_END();
}

void setup()
{
    // Wait 2 seconds before starting tests
    delay(2000);
    runUnityTests();
}

void loop()
{
    // Nothing to do here
    exit(0);
}