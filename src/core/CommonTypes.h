// CommonTypes.h
#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <atomic>
#include <optional>

namespace canaspad
{
    enum class AuthType
    {
        None,
        Basic,
        Bearer
    };

    enum class TimePolicy
    {
        RequireValidTime,
        AutoSync,
        Ignore
    };

    struct ClientOptions
    {
        bool followRedirects = true;
        int maxRedirects = 5;
        int maxRetries = 3;
        int port = 0; // ポート番号 (0 の場合はスキームのデフォルトポートを使用)
        std::chrono::milliseconds retryDelay = std::chrono::seconds(1);
        bool verifySsl = true;
        bool skipTimeCheck = false; // true の場合は TimePolicy::Ignore 相当（既存互換）
        TimePolicy timePolicy = TimePolicy::AutoSync;
        std::string ntpServer = "time.cloudflare.com";
        std::string timeUrl = "https://timestamp.canaspad.net/";
        std::string proxyUrl;
        AuthType authType = AuthType::None;
        std::string username;
        std::string password;
        std::string bearerToken;
        std::string rootCA;
        std::string clientCert;
        std::string clientPrivateKey;
    };

    inline TimePolicy effectiveTimePolicy(const ClientOptions &options)
    {
        if (options.skipTimeCheck)
        {
            return TimePolicy::Ignore;
        }
        return options.timePolicy;
    }
}
