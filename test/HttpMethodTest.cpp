#include "HttpMethodTest.h"
#include <string>

void test_get_method(void)
{
    bool useMock = true;
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, useMock);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
    mockClient->injectResponse(response);

    canaspad::Request request;
    request.setUrl("https://example1.com/init").setMethod(canaspad::HttpMethod::GET);
    auto getResult = client.send(request);

    TEST_ASSERT_TRUE(getResult.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, getResult.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Hello, World!", getResult.value().body.c_str());
}

void test_post_method(void)
{
    bool useMock = true;
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, useMock);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    std::string response = "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(response);

    canaspad::Request request;
    request.setUrl("https://api.example.com/resource")
        .setMethod(canaspad::HttpMethod::POST)
        .setBody("{\"key\":\"value\"}");
    auto result = client.send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(201, result.value().statusCode);
}

void test_put_method(void)
{
    bool useMock = true;
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, useMock);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(response);

    canaspad::Request request;
    request.setUrl("https://api.example.com/resource")
        .setMethod(canaspad::HttpMethod::PUT)
        .setBody("{\"updated\":\"data\"}");
    auto result = client.send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
}

void test_delete_method(void)
{
    bool useMock = true;
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, useMock);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    std::string response = "HTTP/1.1 204 No Content\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(response);

    canaspad::Request request;
    request.setUrl("https://api.example.com/resource/1")
        .setMethod(canaspad::HttpMethod::DELETE);
    auto result = client.send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(204, result.value().statusCode);
}

void test_patch_method(void)
{
    bool useMock = true;
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, useMock);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(response);

    canaspad::Request request;
    request.setUrl("https://api.example.com/resource/1")
        .setMethod(canaspad::HttpMethod::PATCH)
        .setBody("{\"partial\":\"update\"}");
    auto result = client.send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
}

void test_head_method(void)
{
    bool useMock = true;
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, useMock);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(response);

    canaspad::Request request;
    request.setUrl("https://example.com")
        .setMethod(canaspad::HttpMethod::HEAD);
    auto result = client.send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("", result.value().body.c_str());
}

void test_options_method(void)
{
    bool useMock = true;
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, useMock);
    auto *mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client.getConnection());

    std::string response = "HTTP/1.1 200 OK\r\nAllow: GET, POST, HEAD, OPTIONS\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(response);

    canaspad::Request request;
    request.setUrl("https://api.example.com")
        .setMethod(canaspad::HttpMethod::OPTIONS);
    auto result = client.send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);

    auto allowIt = result.value().headers.find("Allow");
    TEST_ASSERT_TRUE(allowIt != result.value().headers.end());
    TEST_ASSERT_EQUAL_STRING("GET, POST, HEAD, OPTIONS", allowIt->second.c_str());
}

void run_http_method_tests(void)
{
    // RUN_TEST(test_get_method);
    RUN_TEST(test_post_method);
    RUN_TEST(test_put_method);
    RUN_TEST(test_delete_method);
    RUN_TEST(test_patch_method);
    RUN_TEST(test_head_method);
    RUN_TEST(test_options_method);
}