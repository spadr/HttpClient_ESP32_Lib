#include "MockWiFiClientSecureTest.h"
#include "../src/core/CommonTypes.h"
#include "../src/core/mock/MockWiFiClientSecure.h"
#include <vector>
#include <string>

void test_mock_reads_two_responses_correctly(void)
{
    canaspad::ClientOptions options;
    canaspad::MockWiFiClientSecure client(options);

    // 1つ目のレスポンスを注入
    std::string response1 = "HTTP/1.1 302 Found\r\nLocation: https://example.com/redirected\r\n\r\n";
    client.injectResponse(std::vector<uint8_t>(response1.begin(), response1.end()));

    // 2つ目のレスポンスを注入
    std::string response2 = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!";
    client.injectResponse(std::vector<uint8_t>(response2.begin(), response2.end()));

    // 接続をシミュレート
    TEST_ASSERT_TRUE(client.connect("example.com", 443));

    // 1つ目のレスポンスを読み取る
    std::vector<uint8_t> buffer1(1024);
    int bytesRead1 = client.read(buffer1.data(), buffer1.size());
    std::string result1(buffer1.begin(), buffer1.begin() + bytesRead1);
    TEST_ASSERT_EQUAL_STRING(response1.c_str(), result1.c_str());

    // 2つ目のレスポンスに移動
    client.moveToNextResponse();

    // 2つ目のレスポンスを読み取る
    std::vector<uint8_t> buffer2(1024);
    int bytesRead2 = client.read(buffer2.data(), buffer2.size());
    std::string result2(buffer2.begin(), buffer2.begin() + bytesRead2);
    TEST_ASSERT_EQUAL_STRING(response2.c_str(), result2.c_str());

    // すべてのデータが読み取られたことを確認
    TEST_ASSERT_EQUAL_INT(0, client.available());
}

void test_mock_handles_multiple_reads(void)
{
    canaspad::ClientOptions options;
    canaspad::MockWiFiClientSecure client(options);

    // レスポンスを注入
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!";
    client.injectResponse(std::vector<uint8_t>(response.begin(), response.end()));

    // 接続をシミュレート
    TEST_ASSERT_TRUE(client.connect("example.com", 443));

    // 複数回の読み取りでレスポンス全体を取得
    std::string result;
    std::vector<uint8_t> buffer(10); // 小さいバッファを使用
    while (client.available() > 0)
    {
        int bytesRead = client.read(buffer.data(), buffer.size());
        result.append(buffer.begin(), buffer.begin() + bytesRead);
    }

    TEST_ASSERT_EQUAL_STRING(response.c_str(), result.c_str());
    TEST_ASSERT_EQUAL_INT(0, client.available());
}

void run_mock_wifi_client_secure_tests(void)
{
    RUN_TEST(test_mock_reads_two_responses_correctly);
    RUN_TEST(test_mock_handles_multiple_reads);
}