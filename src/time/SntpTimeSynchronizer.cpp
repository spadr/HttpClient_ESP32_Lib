#include "TimeSynchronizer.h"

#include "../core/Logger.h"

#ifndef ARDUINO_ARCH_NATIVE
#include <Arduino.h>
#if __has_include(<esp_sntp.h>)
#include <esp_sntp.h>
#define CANASPAD_HAS_ESP_SNTP 1
#elif __has_include(<lwip/apps/sntp.h>)
#include <lwip/apps/sntp.h>
#define CANASPAD_HAS_LWIP_SNTP 1
#endif
#endif

namespace canaspad
{

    SntpTimeSynchronizer::SntpTimeSynchronizer(std::string ntpServer, int timeoutMs, SyncFn syncFn)
        : m_ntpServer(std::move(ntpServer)),
          m_timeoutMs(timeoutMs > 0 ? timeoutMs : 10000),
          m_syncFn(std::move(syncFn))
    {
    }

    const char *SntpTimeSynchronizer::name() const
    {
        return "SNTP";
    }

    Result<void> SntpTimeSynchronizer::synchronize()
    {
        if (m_syncFn)
        {
            return m_syncFn(m_ntpServer);
        }

#ifdef ARDUINO_ARCH_NATIVE
        return Result<void>(ErrorInfo(ErrorCode::TimeSyncFailed,
                                      "SNTP is not available in the native environment"));
#else
        if (m_ntpServer.empty())
        {
            return Result<void>(ErrorInfo(ErrorCode::InvalidOption, "NTP server is empty"));
        }

        LOG_INFO("Starting SNTP sync with %s", m_ntpServer.c_str());

#if defined(CANASPAD_HAS_ESP_SNTP)
        if (!esp_sntp_enabled())
        {
            esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
            esp_sntp_setservername(0, const_cast<char *>(m_ntpServer.c_str()));
            esp_sntp_init();
        }
#elif defined(CANASPAD_HAS_LWIP_SNTP)
        if (!sntp_enabled())
        {
            sntp_setoperatingmode(SNTP_OPMODE_POLL);
            sntp_setservername(0, const_cast<char *>(m_ntpServer.c_str()));
            sntp_init();
        }
#else
        return Result<void>(ErrorInfo(ErrorCode::TimeSyncFailed, "SNTP headers are not available"));
#endif

        const unsigned long start = millis();
        while ((millis() - start) < static_cast<unsigned long>(m_timeoutMs))
        {
            if (isSystemTimeValidForTls(SystemTime::nowUnixSeconds()))
            {
                LOG_INFO_S("SNTP time sync succeeded");
                return Result<void>();
            }
            delay(50);
        }

        return Result<void>(ErrorInfo(ErrorCode::TimeSyncFailed, "SNTP time sync timed out"));
#endif
    }

} // namespace canaspad
