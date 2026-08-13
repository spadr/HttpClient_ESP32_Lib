#ifdef NATIVE_TEST
#include "../../helpers/TestEnvironment.h"
#endif

#include "../../helpers/simple_unity.h"
#include "../../../src/HttpClient.h"
#include "../../../src/core/mock/MockWiFiClientSecure.h"
#include "../../../src/time/TimeSynchronizer.h"
#include "../../helpers/AssertHelpers.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using namespace canaspad;

namespace
{
    int64_t g_fakeNow = 0;
    int64_t g_lastSetTime = 0;
    int g_setTimeCalls = 0;

    constexpr int64_t kValidUnixTime = 1700000000LL;      // 2023-11-14
    constexpr int64_t kUnixTime2038 = 2147483648LL;       // 2038-01-19 03:14:08 UTC
    constexpr int64_t kYear2000UnixTime = 946684800LL;    // 2000-01-01
    constexpr int64_t kOldBugThreshold = 3600LL * 9;      // former (incorrect) "year 2000" check

    class CountingSynchronizer : public TimeSynchronizer
    {
    public:
        int calls = 0;
        bool succeed = true;
        int64_t timeToSet = kValidUnixTime;
        std::string label;

        CountingSynchronizer(std::string name, bool ok, int64_t t = kValidUnixTime)
            : succeed(ok), timeToSet(t), label(std::move(name))
        {
        }

        Result<void> synchronize() override
        {
            ++calls;
            if (!succeed)
            {
                return Result<void>(ErrorInfo(ErrorCode::TimeSyncFailed, label + " failed"));
            }
            return SystemTime::setUnixTime(timeToSet, 0);
        }

        const char *name() const override
        {
            return label.c_str();
        }
    };

    std::string tzValue()
    {
        const char *tz = std::getenv("TZ");
        return tz ? std::string(tz) : std::string();
    }

    void injectOkResponse(HttpClient &client)
    {
        auto *mockClient = static_cast<MockWiFiClientSecure *>(client.getConnection());
        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 2\r\n\r\n"
            "OK";
        mockClient->injectResponse(std::vector<uint8_t>(response, response + std::strlen(response)));
    }

    Request sampleRequest()
    {
        Request request;
        request.setUrl("https://example.com/time-test").setMethod(HttpMethod::GET);
        return request;
    }

    ClientOptions tlsOptions(TimePolicy policy, bool skipTimeCheck = false)
    {
        ClientOptions options;
        options.verifySsl = true;
        options.timePolicy = policy;
        options.skipTimeCheck = skipTimeCheck;
        options.maxRetries = 0;
        return options;
    }

    struct FakeClockGuard
    {
        FakeClockGuard()
        {
            g_fakeNow = 0;
            g_lastSetTime = 0;
            g_setTimeCalls = 0;
            SystemTime::setNowOverride([]()
                                       { return g_fakeNow; });
            SystemTime::setSetTimeOverride([](int64_t seconds, int32_t)
                                           {
                                               ++g_setTimeCalls;
                                               g_lastSetTime = seconds;
                                               g_fakeNow = seconds;
                                               return Result<void>();
                                           });
        }

        ~FakeClockGuard()
        {
            SystemTime::clearOverrides();
        }
    };
}

void setUp(void)
{
}

void tearDown(void)
{
    SystemTime::clearOverrides();
}

void test_epoch_and_y2k_are_not_valid_for_tls()
{
    TEST_ASSERT_FALSE(isSystemTimeValidForTls(0));
    TEST_ASSERT_FALSE(isSystemTimeValidForTls(kOldBugThreshold));
    TEST_ASSERT_FALSE(isSystemTimeValidForTls(kYear2000UnixTime));
    TEST_ASSERT_TRUE(isSystemTimeValidForTls(kMinTlsUsableUnixTime));
    TEST_ASSERT_TRUE(isSystemTimeValidForTls(kValidUnixTime));
    TEST_ASSERT_TRUE(isSystemTimeValidForTls(kUnixTime2038));
    TEST_ASSERT_FALSE(isSystemTimeValidForTls(kMaxTlsUsableUnixTime));
}

void test_parse_unix_timestamp_accepts_2038_and_rejects_garbage()
{
    auto parsed2038 = parseUnixTimestamp("2147483648\n");
    TestHelpers::assertResultSuccess(parsed2038, "2038 timestamp should parse as 64-bit");
    TEST_ASSERT_TRUE(parsed2038.value() == kUnixTime2038);

    auto parsedValid = parseUnixTimestamp(" 1700000000 ");
    TestHelpers::assertResultSuccess(parsedValid, "Trimmed timestamp should parse");
    TEST_ASSERT_TRUE(parsedValid.value() == kValidUnixTime);

    TestHelpers::assertResultError(parseUnixTimestamp(""), ErrorCode::InvalidResponse, "Empty timestamp");
    TestHelpers::assertResultError(parseUnixTimestamp("-1"), ErrorCode::InvalidResponse, "Negative timestamp");
    TestHelpers::assertResultError(parseUnixTimestamp("not-a-number"), ErrorCode::InvalidResponse, "Garbage timestamp");
    TestHelpers::assertResultError(parseUnixTimestamp("1700000000abc"), ErrorCode::InvalidResponse, "Trailing junk");
    TestHelpers::assertResultError(parseUnixTimestamp("123"), ErrorCode::InvalidResponse, "Implausibly small timestamp");
    TestHelpers::assertResultError(parseUnixTimestamp("946684800"), ErrorCode::InvalidResponse, "Y2K timestamp");
}

void test_valid_system_time_does_not_call_synchronizers()
{
    FakeClockGuard clock;
    g_fakeNow = kValidUnixTime;
    auto sntp = std::make_unique<CountingSynchronizer>("sntp", false);
    auto http = std::make_unique<CountingSynchronizer>("http", false);
    auto *sntpPtr = sntp.get();
    auto *httpPtr = http.get();

    TimeSyncManager manager(std::move(sntp), std::move(http));
    auto result = manager.ensureTime(TimePolicy::AutoSync);

    TestHelpers::assertResultSuccess(result, "Valid system time should skip sync");
    TEST_ASSERT_EQUAL_INT(0, sntpPtr->calls);
    TEST_ASSERT_EQUAL_INT(0, httpPtr->calls);
    TEST_ASSERT_EQUAL_INT(0, g_setTimeCalls);
}

void test_invalid_system_time_tries_sntp_first()
{
    FakeClockGuard clock;
    g_fakeNow = 0;
    auto sntp = std::make_unique<CountingSynchronizer>("sntp", true);
    auto http = std::make_unique<CountingSynchronizer>("http", true);
    auto *sntpPtr = sntp.get();
    auto *httpPtr = http.get();

    TimeSyncManager manager(std::move(sntp), std::move(http));
    auto result = manager.ensureTime(TimePolicy::AutoSync);

    TestHelpers::assertResultSuccess(result, "SNTP success should satisfy AutoSync");
    TEST_ASSERT_EQUAL_INT(1, sntpPtr->calls);
    TEST_ASSERT_EQUAL_INT(0, httpPtr->calls);
    TEST_ASSERT_TRUE(g_fakeNow == kValidUnixTime);
}

void test_sntp_failure_falls_back_to_http_api()
{
    FakeClockGuard clock;
    g_fakeNow = 0;
    auto sntp = std::make_unique<CountingSynchronizer>("sntp", false);
    auto http = std::make_unique<CountingSynchronizer>("http", true, kUnixTime2038);
    auto *sntpPtr = sntp.get();
    auto *httpPtr = http.get();

    TimeSyncManager manager(std::move(sntp), std::move(http));
    auto result = manager.ensureTime(TimePolicy::AutoSync);

    TestHelpers::assertResultSuccess(result, "HTTP API fallback should succeed after SNTP failure");
    TEST_ASSERT_EQUAL_INT(1, sntpPtr->calls);
    TEST_ASSERT_EQUAL_INT(1, httpPtr->calls);
    TEST_ASSERT_TRUE(g_fakeNow == kUnixTime2038);
}

void test_sntp_and_http_api_failure_returns_time_sync_failed()
{
    FakeClockGuard clock;
    g_fakeNow = 0;
    auto sntp = std::make_unique<CountingSynchronizer>("sntp", false);
    auto http = std::make_unique<CountingSynchronizer>("http", false);
    auto *sntpPtr = sntp.get();
    auto *httpPtr = http.get();

    TimeSyncManager manager(std::move(sntp), std::move(http));
    auto result = manager.ensureTime(TimePolicy::AutoSync);

    TestHelpers::assertResultError(result, ErrorCode::TimeSyncFailed, "Both sync paths failed");
    TEST_ASSERT_EQUAL_INT(1, sntpPtr->calls);
    TEST_ASSERT_EQUAL_INT(1, httpPtr->calls);
    TEST_ASSERT_TRUE(g_fakeNow == 0);

    auto second = manager.ensureTime(TimePolicy::AutoSync);
    TestHelpers::assertResultError(second, ErrorCode::TimeSyncFailed, "Cooldown should prevent immediate retry");
    TEST_ASSERT_EQUAL_INT(1, sntpPtr->calls);
    TEST_ASSERT_EQUAL_INT(1, httpPtr->calls);
}

void test_require_valid_time_does_not_autosync()
{
    FakeClockGuard clock;
    g_fakeNow = 0;
    auto sntp = std::make_unique<CountingSynchronizer>("sntp", true);
    auto http = std::make_unique<CountingSynchronizer>("http", true);
    auto *sntpPtr = sntp.get();
    auto *httpPtr = http.get();

    TimeSyncManager manager(std::move(sntp), std::move(http));
    auto result = manager.ensureTime(TimePolicy::RequireValidTime);

    TestHelpers::assertResultError(result, ErrorCode::TimeNotSet, "RequireValidTime should not sync");
    TEST_ASSERT_EQUAL_INT(0, sntpPtr->calls);
    TEST_ASSERT_EQUAL_INT(0, httpPtr->calls);
}

void test_http_api_synchronizer_sets_system_time()
{
    FakeClockGuard clock;
    g_fakeNow = 0;
    const std::string tzBefore = tzValue();

    HttpApiTimeSynchronizer synchronizer(
        "https://timestamp.canaspad.net/",
        [](const std::string &url)
        {
            TEST_ASSERT_EQUAL_STRING("https://timestamp.canaspad.net/", url.c_str());
            return Result<std::string>(std::string("2147483648"));
        });

    auto result = synchronizer.synchronize();
    TestHelpers::assertResultSuccess(result, "HTTP API synchronizer should succeed");
    TEST_ASSERT_TRUE(g_lastSetTime == kUnixTime2038);
    TEST_ASSERT_TRUE(g_fakeNow == kUnixTime2038);
    TEST_ASSERT_EQUAL_STRING(tzBefore.c_str(), tzValue().c_str());
}

void test_http_api_synchronizer_rejects_invalid_timestamp()
{
    FakeClockGuard clock;
    HttpApiTimeSynchronizer synchronizer(
        "https://timestamp.canaspad.net/",
        [](const std::string &)
        {
            return Result<std::string>(std::string("not-a-timestamp"));
        });

    auto result = synchronizer.synchronize();
    TestHelpers::assertResultError(result, ErrorCode::InvalidResponse, "Invalid HTTP timestamp");
    TEST_ASSERT_EQUAL_INT(0, g_setTimeCalls);
}

void test_http_client_recovers_after_time_is_set()
{
    FakeClockGuard clock;
    g_fakeNow = 0;
    HttpClient client(tlsOptions(TimePolicy::RequireValidTime), true);
    injectOkResponse(client);

    auto first = client.send(sampleRequest());
    TestHelpers::assertResultError(first, ErrorCode::TimeNotSet, "Invalid time at send should fail");

    g_fakeNow = kValidUnixTime;
    auto second = client.send(sampleRequest());
    TestHelpers::assertResultSuccess(second, "Same client should work after time is set");
    TEST_ASSERT_EQUAL_INT(200, second.value().statusCode);
}

void test_skip_time_check_preserves_legacy_ignore_behavior()
{
    FakeClockGuard clock;
    g_fakeNow = 0;
    auto sntp = std::make_unique<CountingSynchronizer>("sntp", true);
    auto http = std::make_unique<CountingSynchronizer>("http", true);
    auto *sntpPtr = sntp.get();
    auto *httpPtr = http.get();

    HttpClient client(tlsOptions(TimePolicy::AutoSync, true), true);
    client.setTimeSyncManager(std::make_unique<TimeSyncManager>(std::move(sntp), std::move(http)));
    injectOkResponse(client);

    auto result = client.send(sampleRequest());
    TestHelpers::assertResultSuccess(result, "skipTimeCheck should allow send without valid time");
    TEST_ASSERT_EQUAL_INT(0, sntpPtr->calls);
    TEST_ASSERT_EQUAL_INT(0, httpPtr->calls);
}

void test_http_client_autosync_uses_sntp_then_continues()
{
    FakeClockGuard clock;
    g_fakeNow = 0;
    auto sntp = std::make_unique<CountingSynchronizer>("sntp", true);
    auto http = std::make_unique<CountingSynchronizer>("http", false);
    auto *sntpPtr = sntp.get();
    auto *httpPtr = http.get();

    HttpClient client(tlsOptions(TimePolicy::AutoSync), true);
    client.setTimeSyncManager(std::make_unique<TimeSyncManager>(std::move(sntp), std::move(http)));
    injectOkResponse(client);

    auto result = client.send(sampleRequest());
    TestHelpers::assertResultSuccess(result, "AutoSync SNTP success should allow HTTPS send");
    TEST_ASSERT_EQUAL_INT(1, sntpPtr->calls);
    TEST_ASSERT_EQUAL_INT(0, httpPtr->calls);
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
}

void test_http_client_autosync_falls_back_to_http_api()
{
    FakeClockGuard clock;
    g_fakeNow = 0;
    auto sntp = std::make_unique<CountingSynchronizer>("sntp", false);
    auto http = std::make_unique<CountingSynchronizer>("http", true);
    auto *sntpPtr = sntp.get();
    auto *httpPtr = http.get();

    HttpClient client(tlsOptions(TimePolicy::AutoSync), true);
    client.setTimeSyncManager(std::make_unique<TimeSyncManager>(std::move(sntp), std::move(http)));
    injectOkResponse(client);

    auto result = client.send(sampleRequest());
    TestHelpers::assertResultSuccess(result, "AutoSync HTTP fallback should allow HTTPS send");
    TEST_ASSERT_EQUAL_INT(1, sntpPtr->calls);
    TEST_ASSERT_EQUAL_INT(1, httpPtr->calls);
}

void test_http_client_autosync_both_fail()
{
    FakeClockGuard clock;
    g_fakeNow = 0;
    auto sntp = std::make_unique<CountingSynchronizer>("sntp", false);
    auto http = std::make_unique<CountingSynchronizer>("http", false);

    HttpClient client(tlsOptions(TimePolicy::AutoSync), true);
    client.setTimeSyncManager(std::make_unique<TimeSyncManager>(std::move(sntp), std::move(http)));
    injectOkResponse(client);

    auto result = client.send(sampleRequest());
    TestHelpers::assertResultError(result, ErrorCode::TimeSyncFailed, "AutoSync should surface TimeSyncFailed");
}

void test_http_client_sync_time_does_not_change_timezone()
{
    const std::string tzBefore = tzValue();
    TEST_ASSERT_TRUE(HttpClient::syncTime("https://timestamp.canaspad.net/"));
    TEST_ASSERT_EQUAL_STRING(tzBefore.c_str(), tzValue().c_str());
}

void test_effective_time_policy_skip_time_check_overrides_autosync()
{
    ClientOptions options;
    options.timePolicy = TimePolicy::AutoSync;
    options.skipTimeCheck = true;
    TEST_ASSERT_TRUE(effectiveTimePolicy(options) == TimePolicy::Ignore);

    options.skipTimeCheck = false;
    TEST_ASSERT_TRUE(effectiveTimePolicy(options) == TimePolicy::AutoSync);
}

#ifdef NATIVE_TEST
int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_epoch_and_y2k_are_not_valid_for_tls);
    RUN_TEST(test_parse_unix_timestamp_accepts_2038_and_rejects_garbage);
    RUN_TEST(test_valid_system_time_does_not_call_synchronizers);
    RUN_TEST(test_invalid_system_time_tries_sntp_first);
    RUN_TEST(test_sntp_failure_falls_back_to_http_api);
    RUN_TEST(test_sntp_and_http_api_failure_returns_time_sync_failed);
    RUN_TEST(test_require_valid_time_does_not_autosync);
    RUN_TEST(test_http_api_synchronizer_sets_system_time);
    RUN_TEST(test_http_api_synchronizer_rejects_invalid_timestamp);
    RUN_TEST(test_http_client_recovers_after_time_is_set);
    RUN_TEST(test_skip_time_check_preserves_legacy_ignore_behavior);
    RUN_TEST(test_http_client_autosync_uses_sntp_then_continues);
    RUN_TEST(test_http_client_autosync_falls_back_to_http_api);
    RUN_TEST(test_http_client_autosync_both_fail);
    RUN_TEST(test_http_client_sync_time_does_not_change_timezone);
    RUN_TEST(test_effective_time_policy_skip_time_check_overrides_autosync);
    return UNITY_END();
}
#endif
