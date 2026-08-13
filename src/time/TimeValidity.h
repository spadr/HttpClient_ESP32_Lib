#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "../Result.h"

namespace canaspad
{

    // Arduino-ESP32 getLocalTime() treats year > 2016 as "the clock has been set".
    // Unset ESP32 clocks typically sit at Unix epoch (1970) or the RTC epoch (2000),
    // neither of which is usable for TLS certificate verification.
    constexpr int64_t kMinTlsUsableUnixTime = 1483228800LL; // 2017-01-01T00:00:00Z

    // Reject obviously broken far-future values without modeling clock quality.
    constexpr int64_t kMaxTlsUsableUnixTime = 4102444800LL; // 2100-01-01T00:00:00Z

    bool isSystemTimeValidForTls(int64_t unixSeconds);

    Result<int64_t> parseUnixTimestamp(const std::string &text);

    class SystemTime
    {
    public:
        static int64_t nowUnixSeconds();
        static Result<void> setUnixTime(int64_t unixSeconds, int32_t micros);

        static void setNowOverride(std::function<int64_t()> fn);
        static void setSetTimeOverride(std::function<Result<void>(int64_t, int32_t)> fn);
        static void clearOverrides();
    };

} // namespace canaspad
