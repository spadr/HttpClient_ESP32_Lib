#ifdef NATIVE_TEST
#include "../../helpers/TestEnvironment.h"
#endif

#include "../../helpers/simple_unity.h"
#include "../../../src/HttpClient.h"
#include "../../../src/core/mock/MockWiFiClientSecure.h"
#include "../../helpers/AssertHelpers.h"
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

using namespace canaspad;

static void injectHttpResponse(MockWiFiClientSecure *mockClient, const char *response)
{
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_multi_redirect_chain()
{
    ClientOptions options;
    options.verifySsl = false;
    options.followRedirects = true;
    options.maxRedirects = 5;
    HttpClient client(options, true);
    auto *mockClient = static_cast<MockWiFiClientSecure *>(client.getConnection());

    injectHttpResponse(mockClient,
                       "HTTP/1.1 302 Found\r\n"
                       "Location: http://example.com/step2\r\n"
                       "Content-Length: 0\r\n\r\n");
    injectHttpResponse(mockClient,
                       "HTTP/1.1 302 Found\r\n"
                       "Location: http://example.com/final\r\n"
                       "Content-Length: 0\r\n\r\n");
    injectHttpResponse(mockClient,
                       "HTTP/1.1 200 OK\r\n"
                       "Content-Length: 11\r\n\r\n"
                       "Final Body!");

    Request request;
    request.setUrl("http://example.com/start").setMethod(HttpMethod::GET);
    auto result = client.send(request);

    TestHelpers::assertResultSuccess(result, "Multi redirect chain should succeed");
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Final Body!", result.value().body.c_str());
}

void test_redirect_limit_exceeded()
{
    ClientOptions options;
    options.verifySsl = false;
    options.followRedirects = true;
    options.maxRedirects = 1;
    HttpClient client(options, true);
    auto *mockClient = static_cast<MockWiFiClientSecure *>(client.getConnection());

    injectHttpResponse(mockClient,
                       "HTTP/1.1 302 Found\r\n"
                       "Location: http://example.com/step2\r\n"
                       "Content-Length: 0\r\n\r\n");
    injectHttpResponse(mockClient,
                       "HTTP/1.1 302 Found\r\n"
                       "Location: http://example.com/final\r\n"
                       "Content-Length: 0\r\n\r\n");
    injectHttpResponse(mockClient,
                       "HTTP/1.1 200 OK\r\n"
                       "Content-Length: 2\r\n\r\n"
                       "OK");

    Request request;
    request.setUrl("http://example.com/start").setMethod(HttpMethod::GET);
    auto result = client.send(request);

    TestHelpers::assertResultError(result, ErrorCode::TooManyRedirects, "Should fail when redirect limit exceeded");
}

void test_redirect_303_converts_post_to_get()
{
    ClientOptions options;
    options.verifySsl = false;
    options.followRedirects = true;
    HttpClient client(options, true);
    auto *mockClient = static_cast<MockWiFiClientSecure *>(client.getConnection());

    injectHttpResponse(mockClient,
                       "HTTP/1.1 303 See Other\r\n"
                       "Location: http://example.com/result\r\n"
                       "Content-Length: 0\r\n\r\n");
    injectHttpResponse(mockClient,
                       "HTTP/1.1 200 OK\r\n"
                       "Content-Length: 2\r\n\r\n"
                       "OK");

    Request request;
    request.setUrl("http://example.com/submit")
        .setMethod(HttpMethod::POST)
        .setBody("payload=data");
    auto result = client.send(request);

    TestHelpers::assertResultSuccess(result, "303 redirect should succeed");
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);

    bool getRequestFound = false;
    for (const auto &entry : mockClient->getCommunicationLog().getLog())
    {
        if (entry.type == CommunicationLog::Entry::Type::Sent)
        {
            std::string requestStr(entry.data.begin(), entry.data.end());
            if (requestStr.find("GET /result HTTP/1.1") != std::string::npos)
            {
                getRequestFound = true;
                TEST_ASSERT_TRUE(requestStr.find("payload=data") == std::string::npos);
            }
        }
    }
    TEST_ASSERT_TRUE(getRequestFound);
}

void test_chunked_response_decoding()
{
    ClientOptions options;
    options.verifySsl = false;
    HttpClient client(options, true);
    auto *mockClient = static_cast<MockWiFiClientSecure *>(client.getConnection());

    injectHttpResponse(mockClient,
                       "HTTP/1.1 200 OK\r\n"
                       "Transfer-Encoding: chunked\r\n"
                       "\r\n"
                       "5\r\n"
                       "Hello\r\n"
                       "6\r\n"
                       " World\r\n"
                       "0\r\n"
                       "\r\n");

    Request request;
    request.setUrl("http://example.com/chunked").setMethod(HttpMethod::GET);
    auto result = client.send(request);

    TestHelpers::assertResultSuccess(result, "Chunked response should succeed");
    TEST_ASSERT_EQUAL_STRING("Hello World", result.value().body.c_str());
}

void test_send_streaming_callback()
{
    ClientOptions options;
    options.verifySsl = false;
    HttpClient client(options, true);
    auto *mockClient = static_cast<MockWiFiClientSecure *>(client.getConnection());

    injectHttpResponse(mockClient,
                       "HTTP/1.1 200 OK\r\n"
                       "Content-Length: 11\r\n\r\n"
                       "Stream Body");

    Request request;
    request.setUrl("http://example.com/stream").setMethod(HttpMethod::GET);

    std::string streamedBody;
    auto result = client.sendStreaming(request, [&](const char *data, size_t size)
                                       { streamedBody.append(data, size); });

    TestHelpers::assertResultSuccess(result, "Streaming request should succeed");
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_INT(0, result.value().body.size());
    TEST_ASSERT_EQUAL_STRING("Stream Body", streamedBody.c_str());
}

void test_progress_and_body_callbacks()
{
    ClientOptions options;
    options.verifySsl = false;
    HttpClient client(options, true);
    auto *mockClient = static_cast<MockWiFiClientSecure *>(client.getConnection());

    injectHttpResponse(mockClient,
                       "HTTP/1.1 200 OK\r\n"
                       "Content-Length: 4\r\n\r\n"
                       "TEST");

    std::string callbackBody;
    size_t lastReceived = 0;
    size_t lastTotal = 0;
    client.setResponseBodyCallback([&](const char *data, size_t size)
                                   { callbackBody.append(data, size); });
    client.setProgressCallback([&](size_t received, size_t total)
                               {
                                   lastReceived = received;
                                   lastTotal = total;
                               });

    Request request;
    request.setUrl("http://example.com/callbacks").setMethod(HttpMethod::GET);
    auto result = client.send(request);

    TestHelpers::assertResultSuccess(result, "Callback request should succeed");
    TEST_ASSERT_EQUAL_STRING("TEST", result.value().body.c_str());
    TEST_ASSERT_EQUAL_STRING("TEST", callbackBody.c_str());
    TEST_ASSERT_EQUAL_INT(4, lastReceived);
    TEST_ASSERT_EQUAL_INT(4, lastTotal);
}

void test_cancel_request()
{
    ClientOptions options;
    options.verifySsl = false;
    HttpClient client(options, true);
    auto *mockClient = static_cast<MockWiFiClientSecure *>(client.getConnection());
    mockClient->setReadBehavior(ReadBehavior::SlowResponse, std::chrono::milliseconds(500));

    injectHttpResponse(mockClient,
                       "HTTP/1.1 200 OK\r\n"
                       "Content-Length: 100\r\n\r\n"
                       "START");

    Request request;
    request.setUrl("http://example.com/slow").setMethod(HttpMethod::GET);

    std::atomic<bool> cancelDone{false};
    std::thread cancelThread([&]()
                             {
                                 std::this_thread::sleep_for(std::chrono::milliseconds(50));
                                 client.cancel("test");
                                 cancelDone = true;
                             });

    auto result = client.send(request);
    cancelThread.join();

    TEST_ASSERT_TRUE(cancelDone.load());
    TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ErrorCode::RequestCancelled), static_cast<int>(result.error().code));
}

#ifdef NATIVE_TEST
int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_multi_redirect_chain);
    RUN_TEST(test_redirect_limit_exceeded);
    RUN_TEST(test_redirect_303_converts_post_to_get);
    RUN_TEST(test_chunked_response_decoding);
    RUN_TEST(test_send_streaming_callback);
    RUN_TEST(test_progress_and_body_callbacks);
    RUN_TEST(test_cancel_request);
    return UNITY_END();
}
#endif
