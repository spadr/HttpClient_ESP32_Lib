#pragma once
#include <string>
#include <cstdlib>

class TestEnvironment {
public:
    static bool isMockServerAvailable() {
        return checkConnection("localhost", 1080);
    }
    
    static bool isWireMockAvailable() {
        return checkConnection("localhost", 8080);
    }
    
    static bool isCI() {
        return std::getenv("CI") != nullptr;
    }
    
    static bool isRecordingMode() {
        return !isCI() && std::getenv("RECORD_MODE") != nullptr;
    }
    
    static bool isNativeTest() {
        return std::getenv("NATIVE_TEST") != nullptr;
    }
    
private:
    static bool checkConnection(const std::string& host, int port);
};