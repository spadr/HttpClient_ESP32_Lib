#ifndef RETRY_TEST_H
#define RETRY_TEST_H

#include "helpers.h"

void test_http_client_retry_on_network_error();
void test_http_client_retry_max_retries_exceeded();
void run_retry_tests(void);

#endif // RETRY_TEST_H