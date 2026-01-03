#pragma once

#ifdef NATIVE_TEST

#include <iostream>
#include <string>
#include <cstdint>
#include <thread>
#include <chrono>
#include <cstdio>
#include <ctime>

// Arduino互換のSerial mock
class SerialMock {
public:
    void begin(unsigned long baud) { (void)baud; }
    void println(const std::string& str) {
        std::cout << str << std::endl;
    }
    void println(const char* str) {
        std::cout << str << std::endl;
    }
    template<typename T>
    void println(T value) {
        std::cout << value << std::endl;
    }
    template<typename... Args>
    void printf(const char* format, Args... args) {
        std::printf(format, args...);
    }
};

extern SerialMock Serial;

// Arduino互換のdelay関数
inline void delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Arduino互換の型定義
typedef uint8_t byte;

// Arduino.h compatibility
#define ARDUINO_H
#define HIGH 1
#define LOW 0

// Time functions mock
inline void configTime(long gmtOffset_sec, int daylightOffset_sec, const char* server1, const char* server2 = nullptr, const char* server3 = nullptr) {
    // Mock implementation - do nothing
    (void)gmtOffset_sec; (void)daylightOffset_sec; (void)server1; (void)server2; (void)server3;
}

inline bool getLocalTime(struct tm * info, unsigned long ms = 5000) {
    // Mock implementation - return current time
    (void)ms;
    time_t now = time(0);
    *info = *localtime(&now);
    return true;
}

// WiFi mock
#define WL_CONNECTED 3
class WiFiMock {
public:
    static void begin(const char* ssid, const char* password) {
        (void)ssid; (void)password;
    }
    static int status() { return WL_CONNECTED; }
    static std::string localIP() { return "192.168.1.100"; }
};
extern WiFiMock WiFi;

#endif // NATIVE_TEST