#include "HttpMethodTest.h"
#include "SslConnectionTest.h"


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

    UNITY_END();
}

void setup()
{
    delay(2000);
    runUnityTests();
}

void loop()
{
    // テストが終了したら何もしない
}