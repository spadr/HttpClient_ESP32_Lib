#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <atomic>
#include <optional>
#include "core/Request.h"
#include "core/Response.h"
#include "core/HttpResult.h"
#include "core/ConnectionPool.h"
#include "proxy/Proxy.h"
#include "Result.h"
#include "auth/Auth.h"
#include "core/Connection.h" // Connection インターフェースのインクルードを追加

namespace canaspad
{

    class HttpClient
    {
    public:
        HttpClient(const ClientOptions &options = ClientOptions(), bool useMock = false);
        ~HttpClient();

        struct Timeouts
        {
            std::chrono::milliseconds connect{5000};
            std::chrono::milliseconds read{5000};
            std::chrono::milliseconds write{5000};
        };

        void setTimeouts(const Timeouts &timeouts);
        void setConnectionTimeout(std::chrono::milliseconds timeout);
        void setReadTimeout(std::chrono::milliseconds timeout);
        void setWriteTimeout(std::chrono::milliseconds timeout);

        Result<HttpResult> send(const Request &request);
        void cancel(const std::string &requestId);

        void enableCookies(bool enable = true);
        void setProgressCallback(std::function<void(size_t, size_t)> callback);
        void setResponseBodyCallback(std::function<void(const char *, size_t)> callback);

        using ChunkCallback = std::function<void(const char *, size_t)>;
        Result<HttpResult> sendStreaming(const Request &request, ChunkCallback chunkCallback);

        Connection *getConnection() const;

    private:
        std::unique_ptr<ConnectionPool> m_connectionPool;
        std::shared_ptr<Connection> m_mockConnection;
        std::unique_ptr<Auth> m_auth;
        std::unique_ptr<Proxy> m_proxy;
        Timeouts m_timeouts;
        bool m_cookiesEnabled = false;
        ClientOptions m_options;
        std::function<void(size_t, size_t)> m_progressCallback;
        std::function<void(const char *, size_t)> m_responseBodyCallback;
        bool m_useMock = false;

        bool m_isInitialized = true;
        ErrorInfo m_initializationError;

        bool checkTimeout(const std::chrono::steady_clock::time_point &start,
                          const std::chrono::milliseconds &timeout) const;
        Result<HttpResult> sendWithRedirects(const Request &request, int redirectCount = 0);
        Result<HttpResult> sendWithRetries(const Request &request, int retryCount = 0);
        Result<HttpResult> readResponse(Connection *connection, const Request &request);
        Result<HttpResult> handleChunkedResponse(Connection *connection, HttpResult &result);

        std::string buildRequestString(const Request &request);
    };

} // namespace canaspad