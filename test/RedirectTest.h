#ifndef REDIRECT_TEST_H
#define REDIRECT_TEST_H

#include "helpers.h"

void test_http_client_redirect_handling();
void test_http_client_disable_redirect_following();
void test_http_client_too_many_redirects();
void run_redirect_tests(void);

#endif // REDIRECT_TEST_H