#include "SslConnectionTest.h"

const char *TEST_ROOT_CA =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\n"
    "...\n"
    "-----END CERTIFICATE-----\n";

const char *TEST_CLIENT_CERT =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICXDCCAcWgAwIBAgIJALflMGTNr4cGMA0GCSqGSIb3DQEBCwUAMEUxCzAJBgNV\n"
    "...\n"
    "-----END CERTIFICATE-----\n";

const char *TEST_CLIENT_KEY =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDIYZmVnNr6vW1T\n"
    "...\n"
    "-----END PRIVATE KEY-----\n";

void test_ssl_certificate_configuration()
{
    canaspad::ClientOptions options;
    canaspad::HttpClient *client;
    canaspad::MockWiFiClientSecure *mockClient;

    options.rootCA = TEST_ROOT_CA;
    options.verifySsl = true;
    client = new canaspad::HttpClient(options, true);
    mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client->getConnection());

    TEST_ASSERT_TRUE(mockClient->getVerifySsl());
    TEST_ASSERT_TRUE(mockClient->getClientCert().empty());
    TEST_ASSERT_TRUE(mockClient->getClientPrivateKey().empty());
    TEST_ASSERT_EQUAL_STRING(TEST_ROOT_CA, mockClient->getCACert().c_str());
}

void test_https_connection()
{
    canaspad::ClientOptions options;
    canaspad::HttpClient *client;
    canaspad::MockWiFiClientSecure *mockClient;

    options.rootCA = TEST_ROOT_CA;
    options.verifySsl = true;
    client = new canaspad::HttpClient(options, true);
    mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client->getConnection());

    const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("https://example.com").setMethod(canaspad::Request::Method::GET);

    auto result = client->send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Hello, World!", result.value().body.c_str());
}

void test_client_certificate_authentication()
{
    canaspad::ClientOptions options;
    canaspad::HttpClient *client;
    canaspad::MockWiFiClientSecure *mockClient;

    options.rootCA = TEST_ROOT_CA;
    options.clientCert = TEST_CLIENT_CERT;
    options.clientPrivateKey = TEST_CLIENT_KEY;
    options.verifySsl = true;
    client = new canaspad::HttpClient(options, true);
    mockClient = static_cast<canaspad::MockWiFiClientSecure *>(client->getConnection());

    TEST_ASSERT_EQUAL_STRING(TEST_CLIENT_CERT, mockClient->getClientCert().c_str());
    TEST_ASSERT_EQUAL_STRING(TEST_CLIENT_KEY, mockClient->getClientPrivateKey().c_str());

    const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 14\r\n\r\nHello, Client!";
    mockClient->injectResponse(std::vector<uint8_t>(response, response + strlen(response)));

    canaspad::Request request;
    request.setUrl("https://client-auth.example.com").setMethod(canaspad::Request::Method::GET);

    auto result = client->send(request);

    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_EQUAL_STRING("Hello, Client!", result.value().body.c_str());
}

void run_ssl_connection_tests(void)
{
    RUN_TEST(test_ssl_certificate_configuration);
    RUN_TEST(test_https_connection);
    RUN_TEST(test_client_certificate_authentication);
}
