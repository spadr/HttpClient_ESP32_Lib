#include "HttpMethodTest.h"
#include "SslConnectionTest.h"
#include "CookieTest.h"
#include "RedirectTest.h"
#include "RetryTest.h"

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

    run_http_method_tests();
    run_ssl_connection_tests();
    run_cookie_tests();
    run_redirect_tests();
    run_retry_tests();

    UNITY_END();
}

void setup()
{
    delay(2000);
}

void loop()
{
    runUnityTests(); // loop() 関数内でテストを実行
    delay(1000);     // テスト終了後、1秒待機 (任意)
}