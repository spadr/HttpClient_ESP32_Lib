#include <unity.h>
#include "HttpClient.h"
#include <iostream>
#include <string>
#include <vector>

#ifdef IS_NATIVE
#include "../helpers/MockServerHelper.h"

MockServerHelper *mockServer;
canaspad::HttpClient *client;

// ホスト名定義の取得またはデフォルト設定
#ifdef MOCK_SERVER_HOST
std::string HOST = MOCK_SERVER_HOST;
#else
std::string HOST = "localhost";
#endif

#ifdef MOCK_SERVER_PORT
int PORT = MOCK_SERVER_PORT;
#else
int PORT = 1080;
#endif

std::string getBaseUrl()
{
    return "http://" + HOST + ":" + std::to_string(PORT);
}

void setUp(void)
{
    mockServer = new MockServerHelper(HOST, PORT);
    mockServer->reset();

    canaspad::ClientOptions options;
    client = new canaspad::HttpClient(options);
}

void tearDown(void)
{
    delete client;
    delete mockServer;
}

void test_simple_get_request()
{
    mockServer->setupMock("/hello", "GET", "Hello World!", 200);

    canaspad::Request request;
    request.setUrl(getBaseUrl() + "/hello");
    request.setMethod(canaspad::HttpMethod::GET);

    auto result = client->send(request);

    if (result.isError())
        printf("Request failed: %s\n", result.error().message.c_str());
    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Hello World!", result.value().body.c_str());
}

void test_post_request()
{
    mockServer->setupMock("/post", "POST", "Created", 201);

    canaspad::Request request;
    request.setUrl(getBaseUrl() + "/post");
    request.setMethod(canaspad::HttpMethod::POST);
    request.setBody("{\"key\":\"value\"}");
    request.addHeader("Content-Type", "application/json");

    auto result = client->send(request);

    if (result.isError())
        printf("Request failed: %s\n", result.error().message.c_str());
    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL(201, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Created", result.value().body.c_str());
}

void test_404_not_found()
{
    mockServer->setupMock("/missing", "GET", "Not Found", 404);

    canaspad::Request request;
    request.setUrl(getBaseUrl() + "/missing");
    request.setMethod(canaspad::HttpMethod::GET);

    auto result = client->send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL(404, result.value().statusCode);
}

void test_timeout_handling()
{
    // 2秒遅延レスポンス
    mockServer->setupMockWithDelay("/timeout", "GET", "Slow", 200, 2000);

    canaspad::ClientOptions options;
    delete client; // 再初期化
    client = new canaspad::HttpClient(options);

    // 1秒でタイムアウト設定
    client->setReadTimeout(std::chrono::seconds(1));

    canaspad::Request request;
    request.setUrl(getBaseUrl() + "/timeout");
    request.setMethod(canaspad::HttpMethod::GET);

    auto result = client->send(request);

    TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(canaspad::ErrorCode::Timeout, result.error().code);
}

void test_redirect_handling()
{
    mockServer->setupMockRedirect("/redirect", getBaseUrl() + "/target");
    mockServer->setupMock("/target", "GET", "Target Reached", 200);

    canaspad::ClientOptions options;
    options.followRedirects = true;
    delete client;
    client = new canaspad::HttpClient(options);

    canaspad::Request request;
    request.setUrl(getBaseUrl() + "/redirect");
    request.setMethod(canaspad::HttpMethod::GET);

    auto result = client->send(request);

    if (result.isError())
        printf("Request failed: %s\n", result.error().message.c_str());
    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Target Reached", result.value().body.c_str());
}

void test_large_response()
{
    std::string largeBody(2048, 'A'); // 2KB
    mockServer->setupMock("/large", "GET", largeBody, 200);

    canaspad::Request request;
    request.setUrl(getBaseUrl() + "/large");
    request.setMethod(canaspad::HttpMethod::GET);

    auto result = client->send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL(200, result.value().statusCode);
    TEST_ASSERT_EQUAL(2048, result.value().body.length());
}

void test_multi_redirect_chain()
{
    mockServer->setupMockRedirect("/redirect1", getBaseUrl() + "/redirect2");
    mockServer->setupMockRedirect("/redirect2", getBaseUrl() + "/target");
    mockServer->setupMock("/target", "GET", "Chain Complete", 200);

    canaspad::ClientOptions options;
    options.followRedirects = true;
    delete client;
    client = new canaspad::HttpClient(options);

    canaspad::Request request;
    request.setUrl(getBaseUrl() + "/redirect1");
    request.setMethod(canaspad::HttpMethod::GET);

    auto result = client->send(request);

    if (result.isError())
        printf("Request failed: %s\n", result.error().message.c_str());
    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Chain Complete", result.value().body.c_str());
}

void test_chunked_response()
{
    mockServer->setupMockChunked("/chunked", "GET", "Chunked Response Body", 200);

    canaspad::Request request;
    request.setUrl(getBaseUrl() + "/chunked");
    request.setMethod(canaspad::HttpMethod::GET);

    auto result = client->send(request);

    if (result.isError())
        printf("Request failed: %s\n", result.error().message.c_str());
    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Chunked Response Body", result.value().body.c_str());
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_simple_get_request);
    RUN_TEST(test_post_request);
    RUN_TEST(test_404_not_found);
    RUN_TEST(test_timeout_handling);
    RUN_TEST(test_redirect_handling);
    RUN_TEST(test_multi_redirect_chain);
    RUN_TEST(test_chunked_response);
    RUN_TEST(test_large_response);
    UNITY_END();
    return 0;
}
#endif
