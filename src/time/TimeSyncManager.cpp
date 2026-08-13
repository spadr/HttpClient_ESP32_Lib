#include "TimeSynchronizer.h"

#include <memory>

#include "../core/Logger.h"

namespace canaspad
{

    TimeSyncManager::TimeSyncManager(std::unique_ptr<TimeSynchronizer> sntp,
                                     std::unique_ptr<TimeSynchronizer> http,
                                     std::chrono::milliseconds retryCooldown)
        : m_sntp(std::move(sntp)),
          m_http(std::move(http)),
          m_retryCooldown(retryCooldown)
    {
    }

    std::unique_ptr<TimeSyncManager> TimeSyncManager::createDefault(const ClientOptions &options)
    {
        return std::make_unique<TimeSyncManager>(
            std::make_unique<SntpTimeSynchronizer>(options.ntpServer),
            std::make_unique<HttpApiTimeSynchronizer>(options.timeUrl));
    }

    Result<void> TimeSyncManager::ensureTime(TimePolicy policy)
    {
        if (policy == TimePolicy::Ignore)
        {
            return Result<void>();
        }

        if (isSystemTimeValidForTls(SystemTime::nowUnixSeconds()))
        {
            m_hasFailedAttempt = false;
            return Result<void>();
        }

        if (policy == TimePolicy::RequireValidTime)
        {
            return Result<void>(ErrorInfo(
                ErrorCode::TimeNotSet,
                "System time is not set. Synchronize with SNTP or HTTP time API, or use skipTimeCheck / TimePolicy::Ignore."));
        }

        if (m_hasFailedAttempt &&
            m_retryCooldown.count() > 0 &&
            (std::chrono::steady_clock::now() - m_lastFailedAttempt) < m_retryCooldown)
        {
            return Result<void>(ErrorInfo(
                ErrorCode::TimeSyncFailed,
                "Time sync recently failed; waiting before contacting NTP/HTTP time servers again."));
        }

        if (m_sntp)
        {
            LOG_INFO("Attempting time sync via %s", m_sntp->name());
            auto sntpResult = m_sntp->synchronize();
            if (sntpResult.isSuccess() && isSystemTimeValidForTls(SystemTime::nowUnixSeconds()))
            {
                m_hasFailedAttempt = false;
                return Result<void>();
            }

            if (sntpResult.isError())
            {
                LOG_ERROR("SNTP time sync failed: %s", sntpResult.error().message.c_str());
            }
            else
            {
                LOG_ERROR_S("SNTP time sync returned success but system time is still invalid");
            }
        }

        if (m_http)
        {
            LOG_INFO("Attempting time sync via %s", m_http->name());
            auto httpResult = m_http->synchronize();
            if (httpResult.isSuccess() && isSystemTimeValidForTls(SystemTime::nowUnixSeconds()))
            {
                m_hasFailedAttempt = false;
                return Result<void>();
            }

            if (httpResult.isError())
            {
                LOG_ERROR("HTTP API time sync failed: %s", httpResult.error().message.c_str());
            }
            else
            {
                LOG_ERROR_S("HTTP API time sync returned success but system time is still invalid");
            }
        }

        m_hasFailedAttempt = true;
        m_lastFailedAttempt = std::chrono::steady_clock::now();
        return Result<void>(ErrorInfo(
            ErrorCode::TimeSyncFailed,
            "Failed to synchronize system time via SNTP and HTTP time API."));
    }

} // namespace canaspad
