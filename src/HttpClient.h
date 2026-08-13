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
#include "Result.h"
#include "auth/Auth.h"
#include "core/Connection.h"

namespace canaspad
{

    class TimeSyncManager;

    class HttpClient
    {
    public:
        HttpClient(const ClientOptions &options = ClientOptions(), bool useMock = false);
        ~HttpClient();

        // Constants
        static constexpr size_t DEFAULT_BUFFER_SIZE = 4096;
        static constexpr size_t DEFAULT_REQUEST_BUFFER_RESERVE = 1024;

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

        // Compatibility wrapper around HttpApiTimeSynchronizer.
        static bool syncTime(const std::string &timeUrl = "https://timestamp.canaspad.net/");

        void setTimeSyncManager(std::unique_ptr<TimeSyncManager> manager);

        Connection *getConnection() const;

    private:
        std::unique_ptr<ConnectionPool> m_connectionPool;
        std::shared_ptr<Connection> m_mockConnection;
        std::unique_ptr<Auth> m_auth;
        Timeouts m_timeouts;
        bool m_cookiesEnabled = false;
        ClientOptions m_options;
        std::function<void(size_t, size_t)> m_progressCallback;
        std::function<void(const char *, size_t)> m_responseBodyCallback;
        bool m_useMock = false;
        std::unique_ptr<TimeSyncManager> m_timeSyncManager;
        std::atomic<bool> m_cancelled{false};

        struct ReadOptions
        {
            bool streaming;
            ChunkCallback chunkCallback;

            ReadOptions() : streaming(false), chunkCallback(nullptr) {}
        };

        bool checkTimeout(const std::chrono::steady_clock::time_point &start,
                          const std::chrono::milliseconds &timeout) const;
        bool isCancelled() const;
        Result<HttpResult> cancelledResult() const;
        void notifyBodyChunk(const char *data, size_t size, size_t contentLength,
                             const ReadOptions &options, std::string &bodyAccumulator);
        Result<HttpResult> executeRequest(const Request &request);
        Result<HttpResult> executeRequest(const Request &request, ReadOptions options);
        Request buildRedirectRequest(const Request &originalRequest, const HttpResult &redirectResponse, int statusCode);
        bool isSameOrigin(const std::string &url1, const std::string &url2) const;
        void processCookies(const Request &request, HttpResult &httpResult);
        Result<HttpResult> sendWithRedirects(const Request &request);
        Result<HttpResult> sendWithRedirects(const Request &request, ReadOptions options, int redirectCount);
        Result<HttpResult> sendWithRetries(const Request &request, int retryCount = 0);
        Result<HttpResult> sendWithRetries(const Request &request, ReadOptions options, int retryCount);
        Result<std::shared_ptr<Connection>> establishConnection(const Request &request);
        Result<std::shared_ptr<Connection>> establishDirectConnection(std::shared_ptr<Connection> connection, const std::string &host, int port);
        Result<std::shared_ptr<Connection>> establishProxyConnection(std::shared_ptr<Connection> connection, const Request &request);
        Result<std::shared_ptr<Connection>> establishProxyTunnel(std::shared_ptr<Connection> connection, const Request &request, const std::string &proxyHost, int proxyPort);
        Result<HttpResult> readResponse(Connection *connection, const Request &request);
        Result<HttpResult> readResponse(Connection *connection, const Request &request, ReadOptions options);
        Result<HttpResult> handleChunkedResponse(Connection *connection, HttpResult &result, size_t startingPos);
        Result<HttpResult> handleChunkedResponse(Connection *connection, HttpResult &result, size_t startingPos, ReadOptions options);

        void applyConnectionTimeouts(Connection *connection);
        Result<void> ensureTimeForTls();

        std::string buildRequestString(const Request &request);
    };

} // namespace canaspad