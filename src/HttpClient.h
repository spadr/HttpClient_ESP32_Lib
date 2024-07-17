#pragma once

#include <string>
#include <memory>
#include <future>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include "HttpError.h"
#include "Request.h"
#include "Response.h"
#include "NetworkLayer.h"
#include "CookieJar.h"
#include "ILogger.h"
#include "ConnectionPool.h"
#include <Arduino.h>

namespace http
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
        std::shared_ptr<ILogger> logger;
        std::string rootCA;           // ルート証明書
        std::string clientCert;       // クライアント証明書
        std::string clientPrivateKey; // クライアント秘密鍵
    };

    class HttpClient
    {
    public:
        HttpClient(const ClientOptions &options = ClientOptions());
        ~HttpClient();

        INetworkLayer *getNetworkLayer() { return m_networkLayer; }

        std::future<std::unique_ptr<Response>> send(const Request &request);
        void cancel(const std::string &requestId);

        void setConnectionTimeout(std::chrono::milliseconds timeout);
        void setReadTimeout(std::chrono::milliseconds timeout);
        void setWriteTimeout(std::chrono::milliseconds timeout);

        void enableCookies(bool enable = true);
        void setRateLimit(int requestsPerSecond);
        void setProgressCallback(std::function<void(size_t, size_t)> callback);
        void setResponseBodyCallback(std::function<void(const char *, size_t)> callback);

        using ChunkCallback = std::function<void(const char *, size_t)>;
        std::unique_ptr<Response> sendStreaming(const Request &request, ChunkCallback chunkCallback);

        void setProxy(const std::string &host, int port, const std::string &username = "", const std::string &password = "");

    private:
        std::shared_ptr<WiFiSecureClient> m_wifiClient; // WiFiSecureClient の shared_ptr
        INetworkLayer *m_networkLayer;                  // INetworkLayer へのポインタ
        std::unique_ptr<CookieJar> m_cookieJar;
        std::chrono::milliseconds m_connectionTimeout;
        std::chrono::milliseconds m_readTimeout;
        std::chrono::milliseconds m_writeTimeout;
        bool m_cookiesEnabled;
        ClientOptions m_options;
        std::shared_ptr<ILogger> m_logger;
        std::function<void(size_t, size_t)> m_progressCallback;
        std::function<void(const char *, size_t)> m_responseBodyCallback;
        std::unique_ptr<ConnectionPool> m_connectionPool;

        struct ProxySettings
        {
            std::string host;
            int port;
            std::string username;
            std::string password;
        };
        std::optional<ProxySettings> m_proxySettings;

        std::unique_ptr<Response> sendWithRedirects(const Request &request, int redirectCount = 0);
        std::unique_ptr<Response> sendWithRetries(const Request &request, int retryCount = 0);
        std::unique_ptr<Response> sendStreamingWithRetries(const Request &request, ChunkCallback chunkCallback, int retryCount = 0);
        std::unique_ptr<Response> sendStreamingWithRedirects(const Request &request, ChunkCallback chunkCallback, int redirectCount = 0);
        void applyAuthentication(Request &request);
        void logRequest(const Request &request);
        void logResponse(const Response &response);
        std::unique_ptr<Response> readResponse(INetworkLayer *connection, const Request &request);
        std::unique_ptr<Response> readStreamingResponse(INetworkLayer *connection, const Request &request, ChunkCallback chunkCallback);
        void handleChunkedResponse(INetworkLayer *connection, Response &response);
        void parseStatusLine(const std::string &statusLine, Response &response);
        void parseHeader(const std::string &headerLine, Response &response);
        std::string buildRequestString(const Request &request);
        std::string methodToString(Request::Method method);
        std::string extractHost(const std::string &url);
        int extractPort(const std::string &url);
        std::string extractPath(const std::string &url);
        std::string joinStrings(const std::vector<std::string> &strings, const std::string &delimiter);

        void throwNetworkError(const std::string &message);
        void throwTimeoutError(const std::string &message);
        void throwSSLError(const std::string &message);
        void throwInvalidResponseError(const std::string &message);
        void throwTooManyRedirectsError(const std::string &message);

        static std::string base64Encode(const std::string &input);
        static std::string generateBoundary();
    };

} // namespace http