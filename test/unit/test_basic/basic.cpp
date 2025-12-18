#include <unity.h>

#ifdef NATIVE_TEST
#include "../unit/native_arduino_mock.h"
#else
#include <Arduino.h>
#endif

void test_basic_functionality() {
    TEST_ASSERT_EQUAL(1, 1);
}

void test_addition() {
    TEST_ASSERT_EQUAL(4, 2 + 2);
}

void setUp(void) {
    // テスト前の初期化
}

void tearDown(void) {
    // テスト後のクリーンアップ
}

#ifdef NATIVE_TEST
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_basic_functionality);
    RUN_TEST(test_addition);
    return UNITY_END();
}
#else
void setup() {
    delay(2000);
    Serial.begin(115200);
    
    UNITY_BEGIN();
    RUN_TEST(test_basic_functionality);
    RUN_TEST(test_addition);
    UNITY_END();
}

void loop() {
    delay(1000);
}
#endif