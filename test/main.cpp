#include "HttpMethodTest.h"
#include "UrlAndPortTest.h"
#include "SslConnectionTest.h"
#include "CookieTest.h"
#include "RedirectTest.h"
#include "RetryTest.h"
#include "TimeoutTest.h"
#include "ProxyTest.h"
#include "MockWiFiClientSecureTest.h"
#include <unity.h>

void setUp(void)
{
    // 共通のセットアップコード
}

void tearDown(void)
{
    // 共通のクリーンアップコード
}

void runUnityTests()
{
    UNITY_BEGIN();

    run_mock_wifi_client_secure_tests();
    run_http_method_tests();
    run_url_and_port_tests();
    run_ssl_connection_tests();
    run_cookie_tests();
    // run_redirect_tests();
    // run_retry_tests();
    // run_timeout_tests();
    // run_proxy_tests();

    UNITY_END();
}

void setup()
{
    delay(2000);
    runUnityTests(); // setup() 関数内で一度だけテストを実行
}

void loop()
{
    delay(1000); // テスト終了後、1秒待機 (任意)
}