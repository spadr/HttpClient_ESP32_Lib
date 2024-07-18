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

    struct ClientOptions
    {
        bool followRedirects = true;
        int maxRedirects = 5;
        int maxRetries = 3;
        std::chrono::milliseconds retryDelay = std::chrono::seconds(1);
        bool verifySsl = true;
        std::string proxyUrl;
        AuthType authType = AuthType::None;
        std::string username;
        std::string password;
        std::string bearerToken;
        std::string rootCA;
        std::string clientCert;
        std::string clientPrivateKey;
    };
}