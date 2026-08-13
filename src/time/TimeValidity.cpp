#include "TimeValidity.h"

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <limits>

#ifndef ARDUINO_ARCH_NATIVE
#include <sys/time.h>
#endif

#include "../core/Logger.h"

namespace canaspad
{
    namespace
    {
        std::function<int64_t()> g_nowOverride;
        std::function<Result<void>(int64_t, int32_t)> g_setTimeOverride;

        std::string trimCopy(const std::string &text)
        {
            size_t start = 0;
            while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])))
            {
                ++start;
            }

            size_t end = text.size();
            while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])))
            {
                --end;
            }

            return text.substr(start, end - start);
        }
    }

    bool isSystemTimeValidForTls(int64_t unixSeconds)
    {
        return unixSeconds >= kMinTlsUsableUnixTime && unixSeconds < kMaxTlsUsableUnixTime;
    }

    Result<int64_t> parseUnixTimestamp(const std::string &text)
    {
        const std::string token = trimCopy(text);
        if (token.empty())
        {
            return Result<int64_t>(ErrorInfo(ErrorCode::InvalidResponse, "Empty timestamp"));
        }

        if (token[0] == '-')
        {
            return Result<int64_t>(ErrorInfo(ErrorCode::InvalidResponse, "Negative timestamp"));
        }

        errno = 0;
        char *endptr = nullptr;
        const long long parsed = std::strtoll(token.c_str(), &endptr, 10);
        if (endptr == token.c_str() || *endptr != '\0' || errno == ERANGE)
        {
            return Result<int64_t>(ErrorInfo(ErrorCode::InvalidResponse, "Invalid timestamp"));
        }

        const int64_t unixSeconds = static_cast<int64_t>(parsed);
        if (!isSystemTimeValidForTls(unixSeconds))
        {
            return Result<int64_t>(ErrorInfo(ErrorCode::InvalidResponse, "Timestamp out of acceptable range"));
        }

        return Result<int64_t>(unixSeconds);
    }

    int64_t SystemTime::nowUnixSeconds()
    {
        if (g_nowOverride)
        {
            return g_nowOverride();
        }
        return static_cast<int64_t>(std::time(nullptr));
    }

    Result<void> SystemTime::setUnixTime(int64_t unixSeconds, int32_t micros)
    {
        if (g_setTimeOverride)
        {
            return g_setTimeOverride(unixSeconds, micros);
        }

        if (!isSystemTimeValidForTls(unixSeconds))
        {
            return Result<void>(ErrorInfo(ErrorCode::InvalidResponse, "Refusing to apply invalid system time"));
        }

#ifndef ARDUINO_ARCH_NATIVE
        if (unixSeconds > static_cast<int64_t>(std::numeric_limits<time_t>::max()) ||
            unixSeconds < static_cast<int64_t>(std::numeric_limits<time_t>::min()))
        {
            return Result<void>(ErrorInfo(ErrorCode::TimeSyncFailed, "Timestamp exceeds time_t range"));
        }

        struct timeval tv;
        tv.tv_sec = static_cast<time_t>(unixSeconds);
        tv.tv_usec = micros;
        if (settimeofday(&tv, nullptr) != 0)
        {
            return Result<void>(ErrorInfo(ErrorCode::TimeSyncFailed, "settimeofday failed"));
        }
        return Result<void>();
#else
        (void)micros;
        LOG_INFO_S("Native environment cannot set system time without a test override");
        return Result<void>(ErrorInfo(ErrorCode::TimeSyncFailed, "Cannot set system time in native environment"));
#endif
    }

    void SystemTime::setNowOverride(std::function<int64_t()> fn)
    {
        g_nowOverride = std::move(fn);
    }

    void SystemTime::setSetTimeOverride(std::function<Result<void>(int64_t, int32_t)> fn)
    {
        g_setTimeOverride = std::move(fn);
    }

    void SystemTime::clearOverrides()
    {
        g_nowOverride = nullptr;
        g_setTimeOverride = nullptr;
    }

} // namespace canaspad
