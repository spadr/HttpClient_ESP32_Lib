#pragma once
#ifdef NATIVE_TEST
#include "simple_unity.h"
#else
#include <unity.h>
#endif
#include <string>
#include "../../src/Result.h"
#include "../../src/core/HttpResult.h"

namespace TestHelpers {
    
    // HTTP Result specific assertions
    template<typename T>
    void assertResultSuccess(const canaspad::Result<T>& result, const std::string& message = "") {
        if (result.isError()) {
            std::string errorMsg = "Expected success but got error: " + result.error().message;
            if (!message.empty()) {
                errorMsg = message + " - " + errorMsg;
            }
            TEST_FAIL_MESSAGE(errorMsg.c_str());
        }
    }
    
    template<typename T>
    void assertResultError(const canaspad::Result<T>& result, canaspad::ErrorCode expectedCode, const std::string& message = "") {
        if (result.isSuccess()) {
            std::string errorMsg = "Expected error but got success";
            if (!message.empty()) {
                errorMsg = message + " - " + errorMsg;
            }
            TEST_FAIL_MESSAGE(errorMsg.c_str());
        }
        
        if (result.error().code != expectedCode) {
            std::string errorMsg = "Expected error code " + std::to_string(static_cast<int>(expectedCode)) + 
                                 " but got " + std::to_string(static_cast<int>(result.error().code));
            if (!message.empty()) {
                errorMsg = message + " - " + errorMsg;
            }
            TEST_FAIL_MESSAGE(errorMsg.c_str());
        }
    }
    
    void assertHttpStatus(const canaspad::HttpResult& result, int expectedStatusCode, const std::string& message = "") {
        if (result.statusCode != expectedStatusCode) {
            std::string errorMsg = "Expected status code " + std::to_string(expectedStatusCode) + 
                                 " but got " + std::to_string(result.statusCode);
            if (!message.empty()) {
                errorMsg = message + " - " + errorMsg;
            }
            TEST_FAIL_MESSAGE(errorMsg.c_str());
        }
    }
    
    void assertContains(const std::string& haystack, const std::string& needle, const std::string& message = "") {
        if (haystack.find(needle) == std::string::npos) {
            std::string errorMsg = "String '" + haystack + "' does not contain '" + needle + "'";
            if (!message.empty()) {
                errorMsg = message + " - " + errorMsg;
            }
            TEST_FAIL_MESSAGE(errorMsg.c_str());
        }
    }
    
    void assertNotContains(const std::string& haystack, const std::string& needle, const std::string& message = "") {
        if (haystack.find(needle) != std::string::npos) {
            std::string errorMsg = "String '" + haystack + "' contains '" + needle + "'";
            if (!message.empty()) {
                errorMsg = message + " - " + errorMsg;
            }
            TEST_FAIL_MESSAGE(errorMsg.c_str());
        }
    }
}