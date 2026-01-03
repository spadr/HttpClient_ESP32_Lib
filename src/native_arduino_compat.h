#pragma once

#ifdef ARDUINO_ARCH_NATIVE

#include <cstdint>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdarg>
#include <ctime>

// Arduino-compatible types and functions for native testing
typedef uint8_t byte;

// Serial class mock for native testing
class SerialClass {
public:
    void begin(int baudrate) { /* no-op */ }
    void println(const char* str) { printf("%s\n", str); }
    void print(const char* str) { printf("%s", str); }
    void printf(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
};

extern SerialClass Serial;

// Arduino delay function
inline void delay(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Arduino millis function
inline uint32_t millis() {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
}

// Arduino time function
inline time_t time(time_t* timer) {
    return std::time(timer);
}

// WiFi status constants
#define WL_CONNECTED 3

// WiFi class mock
class WiFiClass {
public:
    void begin(const char* ssid, const char* password) { /* no-op */ }
    int status() { return WL_CONNECTED; }
};

extern WiFiClass WiFi;

// configTime function
inline void configTime(long gmtOffset_sec, int daylightOffset_sec, const char* server) {
    /* no-op for native testing */
}

#endif // ARDUINO_ARCH_NATIVE