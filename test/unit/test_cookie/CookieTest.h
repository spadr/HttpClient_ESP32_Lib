#ifndef COOKIE_TEST_H
#define COOKIE_TEST_H

#include "../../helpers/simple_unity.h"
#include "../../../src/cookie/CookieJar.h"
#include "../../../src/HttpClient.h"
#include "../../../src/core/mock/MockWiFiClientSecure.h"

void test_cookie_jar_set_and_get_cookies();
void test_cookie_jar_expired_cookie();
void test_http_client_cookie_handling();
void run_cookie_tests(void);

#endif // COOKIE_TEST_H