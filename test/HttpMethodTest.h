#ifndef HTTP_METHOD_TEST_H
#define HTTP_METHOD_TEST_H

#include "helpers.h"

void test_get_method(void);
void test_post_method(void);
void test_put_method(void);
void test_delete_method(void);
void test_patch_method(void);
void test_head_method(void);
void test_options_method(void);
void run_http_method_tests(void);

#endif // HTTP_METHOD_TEST_H