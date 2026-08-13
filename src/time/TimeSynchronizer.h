#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "../Result.h"
#include "../core/CommonTypes.h"
#include "TimeValidity.h"

namespace canaspad
{

    class TimeSynchronizer
    {
    public:
        virtual ~TimeSynchronizer() = default;
        virtual Result<void> synchronize() = 0;
        virtual const char *name() const = 0;
    };

    class SntpTimeSynchronizer : public TimeSynchronizer
    {
    public:
        using SyncFn = std::function<Result<void>(const std::string &ntpServer)>;

        explicit SntpTimeSynchronizer(std::string ntpServer = "time.cloudflare.com",
                                      int timeoutMs = 10000,
                                      SyncFn syncFn = nullptr);

        Result<void> synchronize() override;
        const char *name() const override;

    private:
        std::string m_ntpServer;
        int m_timeoutMs;
        SyncFn m_syncFn;
    };

    class HttpApiTimeSynchronizer : public TimeSynchronizer
    {
    public:
        using FetchFn = std::function<Result<std::string>(const std::string &url)>;
        using SetTimeFn = std::function<Result<void>(int64_t unixSeconds, int32_t micros)>;

        explicit HttpApiTimeSynchronizer(std::string timeUrl = "https://timestamp.canaspad.net/",
                                         FetchFn fetch = nullptr,
                                         SetTimeFn setTime = nullptr);

        Result<void> synchronize() override;
        const char *name() const override;

    private:
        std::string m_timeUrl;
        FetchFn m_fetch;
        SetTimeFn m_setTime;
    };

    class TimeSyncManager
    {
    public:
        TimeSyncManager(std::unique_ptr<TimeSynchronizer> sntp,
                        std::unique_ptr<TimeSynchronizer> http,
                        std::chrono::milliseconds retryCooldown = std::chrono::minutes(1));

        static std::unique_ptr<TimeSyncManager> createDefault(const ClientOptions &options);

        Result<void> ensureTime(TimePolicy policy);

    private:
        std::unique_ptr<TimeSynchronizer> m_sntp;
        std::unique_ptr<TimeSynchronizer> m_http;
        std::chrono::milliseconds m_retryCooldown;
        std::chrono::steady_clock::time_point m_lastFailedAttempt{};
        bool m_hasFailedAttempt = false;
    };

} // namespace canaspad
