#ifndef SSL_CONNECTION_TEST_H
#define SSL_CONNECTION_TEST_H

#include "helpers.h"

extern const char *TEST_ROOT_CA;
extern const char *TEST_CLIENT_CERT;
extern const char *TEST_CLIENT_KEY;

void test_ssl_certificate_configuration();
void test_https_connection();
void test_client_certificate_authentication();
void run_ssl_connection_tests(void);

#endif // SSL_CONNECTION_TEST_H