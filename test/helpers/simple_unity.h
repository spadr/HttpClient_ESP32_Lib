#pragma once

#include <iostream>
#include <string>
#include <cstdlib>

// Simple Unity-compatible test framework for native testing
static int test_count = 0;
static int passed_count = 0;
static int failed_count = 0;

#define UNITY_BEGIN() \
    std::cout << "Running tests...\n"; \
    test_count = 0; \
    passed_count = 0; \
    failed_count = 0;

inline int UNITY_END() { \
    std::cout << "\nTest Results:\n"; \
    std::cout << "Tests run: " << test_count << "\n"; \
    std::cout << "Passed: " << passed_count << "\n"; \
    std::cout << "Failed: " << failed_count << "\n"; \
    if (failed_count > 0) { \
        std::cout << "FAILURE\n"; \
        return 1; \
    } else { \
        std::cout << "SUCCESS\n"; \
        return 0; \
    } \
}

#define RUN_TEST(test_func) \
    test_count++; \
    std::cout << "Running " << #test_func << "... "; \
    try { \
        test_func(); \
        std::cout << "PASSED\n"; \
        passed_count++; \
    } catch (const std::exception& e) { \
        std::cout << "FAILED: " << e.what() << "\n"; \
        failed_count++; \
    } catch (...) { \
        std::cout << "FAILED: Unknown exception\n"; \
        failed_count++; \
    }

#define TEST_ASSERT_EQUAL_STRING(expected, actual) \
    if (std::string(expected) != std::string(actual)) { \
        throw std::runtime_error("Expected '" + std::string(expected) + "' but got '" + std::string(actual) + "'"); \
    }

#define TEST_ASSERT_EQUAL_INT(expected, actual) \
    if ((expected) != (actual)) { \
        throw std::runtime_error("Expected " + std::to_string(expected) + " but got " + std::to_string(actual)); \
    }

#define TEST_ASSERT_TRUE(condition) \
    if (!(condition)) { \
        throw std::runtime_error("Expected true but got false"); \
    }

#define TEST_ASSERT_FALSE(condition) \
    if (condition) { \
        throw std::runtime_error("Expected false but got true"); \
    }

#define TEST_ASSERT_NOT_EQUAL_STRING(expected, actual) \
    if (std::string(expected) == std::string(actual)) { \
        throw std::runtime_error("Expected different from '" + std::string(expected) + "'"); \
    }

#define TEST_ASSERT_GREATER_OR_EQUAL_INT(expected, actual) \
    if ((actual) < (expected)) { \
        throw std::runtime_error("Expected " + std::to_string(actual) + " >= " + std::to_string(expected)); \
    }

#define TEST_ASSERT_GREATER_THAN_INT(expected, actual) \
    if ((actual) <= (expected)) { \
        throw std::runtime_error("Expected " + std::to_string(actual) + " > " + std::to_string(expected)); \
    }

#define TEST_ASSERT_NOT_NULL(pointer) \
    if ((pointer) == nullptr) { \
        throw std::runtime_error("Expected non-null pointer"); \
    }

#define TEST_FAIL_MESSAGE(message) \
    throw std::runtime_error(message);

#define TEST_IGNORE_MESSAGE(message) \
    std::cout << "IGNORED: " << message << "\n"; \
    return;