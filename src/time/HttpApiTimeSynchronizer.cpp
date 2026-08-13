#include "TimeSynchronizer.h"

#include "../HttpClient.h"
#include "../core/Logger.h"
#include "../core/Request.h"
#include "../utils/HttpMethod.h"

#include <string>

#ifdef ARDUINO_ARCH_NATIVE
#include "../native_arduino_compat.h"
#else
#include <Arduino.h>
#endif

namespace canaspad
{
    namespace
    {
        Result<std::string> defaultHttpTimeApiFetch(const std::string &url)
        {
            // Bootstrap-only: TLS certificate time checks cannot run until the clock
            // is set. This HttpClient is a one-shot helper and does not change the
            // caller's verifySsl setting.
            ClientOptions options;
            options.verifySsl = false;
            options.skipTimeCheck = true;
            options.timePolicy = TimePolicy::Ignore;
            options.followRedirects = true;

            HttpClient client(options, false);

            Request request;
            request.setUrl(url)
                .setMethod(HttpMethod::GET)
                .addHeader("User-Agent", "HttpClient-ESP32-Lib/1.0.0");

            auto result = client.send(request);
            if (result.isError())
            {
                return Result<std::string>(result.error());
            }

            const auto &response = result.value();
            if (response.statusCode != 200)
            {
                return Result<std::string>(ErrorInfo(
                    ErrorCode::InvalidResponse,
                    "HTTP time API returned status " + std::to_string(response.statusCode)));
            }

            return Result<std::string>(response.body);
        }
    }

    HttpApiTimeSynchronizer::HttpApiTimeSynchronizer(std::string timeUrl, FetchFn fetch, SetTimeFn setTime)
        : m_timeUrl(std::move(timeUrl)),
          m_fetch(std::move(fetch)),
          m_setTime(std::move(setTime))
    {
    }

    const char *HttpApiTimeSynchronizer::name() const
    {
        return "HTTP-API";
    }

    Result<void> HttpApiTimeSynchronizer::synchronize()
    {
        LOG_INFO("Synchronizing time via HTTP API: %s", m_timeUrl.c_str());

        const FetchFn fetch = m_fetch ? m_fetch : defaultHttpTimeApiFetch;
        const unsigned long startMillis = millis();
        auto fetchResult = fetch(m_timeUrl);
        const unsigned long endMillis = millis();
        if (fetchResult.isError())
        {
            return Result<void>(fetchResult.error());
        }

        auto parsed = parseUnixTimestamp(fetchResult.value());
        if (parsed.isError())
        {
            LOG_ERROR_S("Failed to parse HTTP time API response");
            return Result<void>(parsed.error());
        }

        const unsigned long rtt = endMillis - startMillis;
        const int64_t latencyMillis = static_cast<int64_t>(rtt / 2);
        const int64_t adjusted = parsed.value() + (latencyMillis / 1000);
        const int32_t micros = static_cast<int32_t>((latencyMillis % 1000) * 1000);

        if (!isSystemTimeValidForTls(adjusted))
        {
            return Result<void>(ErrorInfo(ErrorCode::InvalidResponse, "Adjusted timestamp is not usable for TLS"));
        }

        Result<void> setResult = m_setTime
                                     ? m_setTime(adjusted, micros)
                                     : SystemTime::setUnixTime(adjusted, micros);
        if (setResult.isError())
        {
            return setResult;
        }

        LOG_INFO("HTTP API time synchronized: %s (RTT: %lu ms)",
                 std::to_string(adjusted).c_str(),
                 static_cast<unsigned long>(rtt));
        return Result<void>();
    }

} // namespace canaspad
