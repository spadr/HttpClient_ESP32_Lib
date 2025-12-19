#pragma once

#include <iostream>
#include <string>
#include <cstdlib>

// Simple Unity-compatible test framework for native testing
// Designed to mimic Unity's output format so PlatformIO can parse it
static int test_count = 0;
static int passed_count = 0;
static int failed_count = 0;

#define UNITY_BEGIN() \
    test_count = 0;   \
    passed_count = 0; \
    failed_count = 0;

inline int UNITY_END()
{
    std::cout << "\n-----------------------\n";
    std::cout << test_count << " Tests " << failed_count << " Failures 0 Ignored\n";
    if (failed_count > 0)
    {
        std::cout << "FAIL\n";
        return 1;
    }
    else
    {
        std::cout << "OK\n";
        return 0;
    }
}

#define RUN_TEST(test_func)                                                            \
    test_count++;                                                                      \
    try                                                                                \
    {                                                                                  \
        test_func();                                                                   \
        std::cout << __FILE__ << ":0:" << #test_func << ":PASS\n";                     \
        passed_count++;                                                                \
    }                                                                                  \
    catch (const std::exception &e)                                                    \
    {                                                                                  \
        std::cout << __FILE__ << ":0:" << #test_func << ":FAIL: " << e.what() << "\n"; \
        failed_count++;                                                                \
    }                                                                                  \
    catch (...)                                                                        \
    {                                                                                  \
        std::cout << __FILE__ << ":0:" << #test_func << ":FAIL: Unknown exception\n";  \
        failed_count++;                                                                \
    }

#define TEST_ASSERT_EQUAL_STRING(expected, actual)                                                                  \
    if (std::string(expected) != std::string(actual))                                                               \
    {                                                                                                               \
        throw std::runtime_error("Expected '" + std::string(expected) + "' but got '" + std::string(actual) + "'"); \
    }

#define TEST_ASSERT_EQUAL_INT(expected, actual)                                                                  \
    if ((expected) != (actual))                                                                                  \
    {                                                                                                            \
        throw std::runtime_error("Expected " + std::to_string(expected) + " but got " + std::to_string(actual)); \
    }

#define TEST_ASSERT_TRUE(condition)                              \
    if (!(condition))                                            \
    {                                                            \
        throw std::runtime_error("Expected true but got false"); \
    }

#define TEST_ASSERT_FALSE(condition)                             \
    if (condition)                                               \
    {                                                            \
        throw std::runtime_error("Expected false but got true"); \
    }

#define TEST_ASSERT_NOT_EQUAL_STRING(expected, actual)                                       \
    if (std::string(expected) == std::string(actual))                                        \
    {                                                                                        \
        throw std::runtime_error("Expected different from '" + std::string(expected) + "'"); \
    }

#define TEST_ASSERT_GREATER_OR_EQUAL_INT(expected, actual)                                                  \
    if ((actual) < (expected))                                                                              \
    {                                                                                                       \
        throw std::runtime_error("Expected " + std::to_string(actual) + " >= " + std::to_string(expected)); \
    }

#define TEST_ASSERT_GREATER_THAN_INT(expected, actual)                                                     \
    if ((actual) <= (expected))                                                                            \
    {                                                                                                      \
        throw std::runtime_error("Expected " + std::to_string(actual) + " > " + std::to_string(expected)); \
    }

#define TEST_ASSERT_NOT_NULL(pointer)                          \
    if ((pointer) == nullptr)                                  \
    {                                                          \
        throw std::runtime_error("Expected non-null pointer"); \
    }

#define TEST_FAIL_MESSAGE(message) \
    throw std::runtime_error(message);

#define TEST_IGNORE_MESSAGE(message)                           \
    std::cout << __FILE__ << ":0:IGNORED:" << message << "\n"; \
    return;