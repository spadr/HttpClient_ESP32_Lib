#include "HttpMethodTest.h"

void test_get_method(void)
{
    canaspad::ClientOptions options;
    canaspad::HttpClient *client;
    canaspad::MockWiFiClientSecure *mockClient;

    options.verifySsl = false;
    client = new canaspad::HttpClient(options, true);
    mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client->getConnection());

    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request getRequest;
    getRequest.setUrl("https://api.example.com/resource")
        .setMethod(canaspad::Request::Method::GET);
    auto getResult = client->send(getRequest);
    TEST_ASSERT_TRUE(getResult.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, getResult.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Hello, World!", getResult.value().body.c_str());
}

void test_post_method(void)
{
    canaspad::ClientOptions options;
    canaspad::HttpClient *client;
    canaspad::MockWiFiClientSecure *mockClient;

    options.verifySsl = false;
    client = new canaspad::HttpClient(options, true);
    mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client->getConnection());

    const char *response = "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request postRequest;
    postRequest.setUrl("https://api.example.com/resource")
        .setMethod(canaspad::Request::Method::POST)
        .setBody("{\"key\":\"value\"}");
    auto postResult = client->send(postRequest);
    TEST_ASSERT_TRUE(postResult.isSuccess());
    TEST_ASSERT_EQUAL_INT(201, postResult.value().statusCode);
}

void test_put_method(void)
{
    canaspad::ClientOptions options;
    canaspad::HttpClient *client;
    canaspad::MockWiFiClientSecure *mockClient;

    options.verifySsl = false;
    client = new canaspad::HttpClient(options, true);
    mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client->getConnection());

    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request putRequest;
    putRequest.setUrl("https://api.example.com/resource")
        .setMethod(canaspad::Request::Method::PUT)
        .setBody("{\"updated\":\"data\"}");
    auto putResult = client->send(putRequest);
    TEST_ASSERT_TRUE(putResult.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, putResult.value().statusCode);
}

void test_delete_method(void)
{
    canaspad::ClientOptions options;
    canaspad::HttpClient *client;
    canaspad::MockWiFiClientSecure *mockClient;

    options.verifySsl = false;
    client = new canaspad::HttpClient(options, true);
    mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client->getConnection());

    const char *response = "HTTP/1.1 204 No Content\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request deleteRequest;
    deleteRequest.setUrl("https://api.example.com/resource/1")
        .setMethod(canaspad::Request::Method::DELETE);
    auto deleteResult = client->send(deleteRequest);
    TEST_ASSERT_TRUE(deleteResult.isSuccess());
    TEST_ASSERT_EQUAL_INT(204, deleteResult.value().statusCode);
}

void test_patch_method(void)
{
    canaspad::ClientOptions options;
    canaspad::HttpClient *client;
    canaspad::MockWiFiClientSecure *mockClient;

    options.verifySsl = false;
    client = new canaspad::HttpClient(options, true);
    mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client->getConnection());

    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request patchRequest;
    patchRequest.setUrl("https://api.example.com/resource/1")
        .setMethod(canaspad::Request::Method::PATCH)
        .setBody("{\"partial\":\"update\"}");
    auto patchResult = client->send(patchRequest);
    TEST_ASSERT_TRUE(patchResult.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, patchResult.value().statusCode);
}

void test_head_method(void)
{
    canaspad::ClientOptions options;
    canaspad::HttpClient *client;
    canaspad::MockWiFiClientSecure *mockClient;

    options.verifySsl = false;
    client = new canaspad::HttpClient(options, true);
    mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client->getConnection());

    const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request headRequest;
    headRequest.setUrl("https://example.com")
        .setMethod(canaspad::Request::Method::HEAD);
    auto headResult = client->send(headRequest);
    TEST_ASSERT_TRUE(headResult.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, headResult.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("", headResult.value().body.c_str());
}

void test_options_method(void)
{
    canaspad::ClientOptions options;
    canaspad::HttpClient *client;
    canaspad::MockWiFiClientSecure *mockClient;

    options.verifySsl = false;
    client = new canaspad::HttpClient(options, true);
    mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client->getConnection());

    const char *response = "HTTP/1.1 200 OK\r\nAllow: GET, POST, HEAD, OPTIONS\r\nContent-Length: 0\r\n\r\n";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request optionsRequest;
    optionsRequest.setUrl("https://api.example.com")
        .setMethod(canaspad::Request::Method::OPTIONS);
    auto optionsResult = client->send(optionsRequest);
    TEST_ASSERT_TRUE(optionsResult.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, optionsResult.value().statusCode);

    auto allowIt = optionsResult.value().headers.find("Allow");
    TEST_ASSERT_TRUE(allowIt != optionsResult.value().headers.end());
    TEST_ASSERT_EQUAL_STRING("GET, POST, HEAD, OPTIONS", allowIt->second.c_str());
}

void run_http_method_tests(void)
{
    RUN_TEST(test_get_method);
    RUN_TEST(test_post_method);
    RUN_TEST(test_put_method);
    RUN_TEST(test_delete_method);
    RUN_TEST(test_patch_method);
    RUN_TEST(test_head_method);
    RUN_TEST(test_options_method);
}
