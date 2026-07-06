#include "../HttpClient.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>
#include "../utils/Utils.h"
#include "WiFiSecureConnection.h"
#include "mock/MockWiFiClientSecure.h"
#ifdef ARDUINO_ARCH_NATIVE
#include "NativeSocketConnection.h"
#endif
#include "RequestValidator.h"
#include "Logger.h"
#include <iostream>
#include <cctype>

// Static constexpr member definitions (required for C++14/17 compatibility)
constexpr size_t canaspad::HttpClient::DEFAULT_BUFFER_SIZE;
constexpr size_t canaspad::HttpClient::DEFAULT_REQUEST_BUFFER_RESERVE;
#ifndef ARDUINO_ARCH_NATIVE
#include <Arduino.h>
#include <sys/time.h>
#else
#include "../native_arduino_compat.h"
#endif

namespace canaspad
{

    namespace
    {
        std::string findHeaderValue(const std::unordered_map<std::string, std::string> &headers, const std::string &key)
        {
            for (const auto &entry : headers)
            {
                if (Utils::caseInsensitiveCompare(entry.first, key))
                {
                    return entry.second;
                }
            }
            return "";
        }

        bool isChunkedEncoding(const std::unordered_map<std::string, std::string> &headers)
        {
            const std::string transferEncoding = findHeaderValue(headers, "Transfer-Encoding");
            std::string lower = transferEncoding;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return lower.find("chunked") != std::string::npos;
        }

        bool shouldConvertToGetOnRedirect(int statusCode, HttpMethod method)
        {
            if (method != HttpMethod::POST)
            {
                return false;
            }
            return statusCode == 301 || statusCode == 302 || statusCode == 303;
        }
    }

    // Static implementation of time synchronization
    bool HttpClient::syncTime(const std::string &timeUrl)
    {
#ifdef ARDUINO_ARCH_NATIVE
        LOG_INFO_S("Native environment - skipping time sync");
        return true;
#else
        LOG_INFO_S("Synchronizing time via HTTP...");

        // We must disable SSL verification for time sync because
        // correct time is needed to verify certificates!
        ClientOptions options;
        options.verifySsl = false;
        options.skipTimeCheck = true; // Allow connection even if time is not set
        options.followRedirects = true;

        HttpClient client(options, false);

        Request request;
        request.setUrl(timeUrl)
            .setMethod(HttpMethod::GET);

        // 時刻同期サーバーはUser-Agentを要求するため追加
        // HttpClient-ESP32-Lib/1.0.0
        request.addHeader("User-Agent", "HttpClient-ESP32-Lib/1.0.0");

        // Measure round-trip time for better accuracy
        unsigned long startMillis = millis();
        auto result = client.send(request);
        unsigned long endMillis = millis();

        if (result.isSuccess())
        {
            auto response = result.value();
            if (response.statusCode == 200)
            {
                try
                {
                    long serverTimestamp = std::stol(response.body);
                    if (serverTimestamp > 1000000000)
                    { // Basic sanity check (after year 2001)
                        // Calculate latency compensation (assume symmetric network delay)
                        // Latency = (RTT) / 2
                        long latencyMillis = (endMillis - startMillis) / 2;

                        struct timeval tv;
                        tv.tv_sec = serverTimestamp + (latencyMillis / 1000);
                        tv.tv_usec = (latencyMillis % 1000) * 1000;
                        settimeofday(&tv, NULL);

                        // Set timezone
                        setenv("TZ", "JST-9", 1);
                        tzset();

                        LOG_INFO("Time synchronized: %ld (Latency: %ld ms)", tv.tv_sec, latencyMillis * 2);
                        return true;
                    }
                }
                catch (...)
                {
                    LOG_ERROR_S("Failed to parse time response");
                }
            }
            else
            {
                LOG_ERROR("Time sync failed. Status: %d", response.statusCode);
            }
        }
        else
        {
            LOG_ERROR("Time sync connection failed: %s", result.error().message.c_str());
        }

        return false;
#endif
    }

    HttpClient::HttpClient(const ClientOptions &options, bool useMock)
        : m_connectionPool(std::make_unique<ConnectionPool>(
              options,
              useMock ? std::shared_ptr<Connection>(new MockWiFiClientSecure(options))
                      :
#ifdef ARDUINO_ARCH_NATIVE
                      std::shared_ptr<Connection>(new NativeSocketConnection())
#else
                      std::shared_ptr<Connection>(new WiFiSecureConnection())
#endif
                  )),
          m_auth(std::make_unique<Auth>(options)),
          m_isInitialized(true),
          m_initializationError(ErrorCode::None, ""),
          m_useMock(useMock),
          m_options(options)
    {
        if (useMock)
        {
            m_mockConnection = std::make_shared<MockWiFiClientSecure>(options);
        }
        else
        {
            // m_connectionPool は初期化子リストですでに初期化されているため再作成不要
            // m_connectionPool = std::make_unique<ConnectionPool>(options);

#ifndef ARDUINO_ARCH_NATIVE
            time_t now;
            time(&now);
            // 時刻未設定（2000年以前）かつ、時刻チェックが有効で、かつSSL検証が有効な場合のみエラーとする
            // 時刻同期のためのHTTP通信(SSL検証なし)などを許可するため
            if (!options.skipTimeCheck && options.verifySsl && now < 3600 * 9)
            {
                m_isInitialized = false;
                m_initializationError = (ErrorInfo(
                    ErrorCode::TimeNotSet,
                    "System time is not set. Please synchronize with NTP server or use skipTimeCheck option."));
            }
#endif
        }
    }

    HttpClient::~HttpClient() = default;

    Connection *HttpClient::getConnection() const
    {
        if (m_useMock)
        {
            return m_mockConnection.get();
        }
        return m_connectionPool->getDefaultConnection();
    }

    bool HttpClient::checkTimeout(const std::chrono::steady_clock::time_point &start,
                                  const std::chrono::milliseconds &timeout) const
    {
        return (std::chrono::steady_clock::now() - start) > timeout;
    }

    Result<HttpResult> HttpClient::send(const Request &request)
    {
        LOG_DEBUG_S("HttpClient::send called");
        LOG_DEBUG("Initial request URL: %s", request.getUrl().c_str());
        if (!m_isInitialized)
        {
            return Result<HttpResult>(std::move(m_initializationError));
        }

        m_cancelled = false;
        return sendWithRetries(request);
    }

    bool HttpClient::isCancelled() const
    {
        return m_cancelled.load();
    }

    Result<HttpResult> HttpClient::cancelledResult() const
    {
        return Result<HttpResult>(ErrorInfo(ErrorCode::RequestCancelled, "Request was cancelled"));
    }

    void HttpClient::notifyBodyChunk(const char *data, size_t size, size_t contentLength,
                                     const ReadOptions &options, std::string &bodyAccumulator)
    {
        if (size == 0)
        {
            return;
        }

        if (options.streaming && options.chunkCallback)
        {
            options.chunkCallback(data, size);
        }
        else
        {
            if (m_responseBodyCallback)
            {
                m_responseBodyCallback(data, size);
            }
            bodyAccumulator.append(data, size);
        }

        if (m_progressCallback && contentLength > 0)
        {
            m_progressCallback(bodyAccumulator.size(), contentLength);
        }
    }

    Result<HttpResult> HttpClient::sendWithRetries(const Request &request, int retryCount)
    {
        return sendWithRetries(request, ReadOptions(), retryCount);
    }

    Result<HttpResult> HttpClient::sendWithRetries(const Request &request, ReadOptions options, int retryCount)
    {
        LOG_DEBUG("HttpClient::sendWithRetries called. Retry count: %d", retryCount);
        LOG_DEBUG("Request URL: %s", request.getUrl().c_str());
        auto result = sendWithRedirects(request, options, 0);

        if (result.isError())
        {
            const auto &error = result.error();
            LOG_ERROR("Error encountered: Code %d, Message: %s", static_cast<int>(error.code), error.message.c_str());

            if (error.code == ErrorCode::RequestCancelled)
            {
                return result;
            }

            // Timeout の場合もリトライ対象に含める
            if ((error.code == ErrorCode::NetworkError || error.code == ErrorCode::Timeout) &&
                retryCount < m_options.maxRetries)
            {
                LOG_INFO_S("Retrying request...");
                std::this_thread::sleep_for(m_options.retryDelay);
                return sendWithRetries(request, options, retryCount + 1);
            }
        }
        return result;
    }

    void HttpClient::processCookies(const Request &request, HttpResult &httpResult)
    {
        if (!m_cookiesEnabled)
        {
            return;
        }

        LOG_DEBUG_S("HttpClient::processCookies - Processing cookies");
        for (const auto &setCookieHeader : Utils::extractHeaders(httpResult.headers, "Set-Cookie"))
        {
            Cookie cookie;
            Utils::parseCookie(setCookieHeader, cookie, request.getUrl());
            httpResult.cookies.push_back(cookie);
            m_connectionPool->getCookieJar()->setCookie(request.getUrl(), setCookieHeader);
        }
    }

    bool HttpClient::isSameOrigin(const std::string &url1, const std::string &url2) const
    {
        return Utils::extractScheme(url1) == Utils::extractScheme(url2) &&
               Utils::extractHost(url1) == Utils::extractHost(url2) &&
               Utils::extractPort(url1) == Utils::extractPort(url2);
    }

    Request HttpClient::buildRedirectRequest(const Request &originalRequest, const HttpResult &redirectResponse, int statusCode)
    {
        auto location = findHeaderValue(redirectResponse.headers, "Location");
        if (location.find("://") == std::string::npos)
        {
            std::string baseUrl = Utils::extractBaseUrl(originalRequest.getUrl());
            if (!location.empty() && location[0] != '/')
            {
                location = baseUrl + "/" + location;
            }
            else
            {
                location = baseUrl + location;
            }
        }

        LOG_DEBUG("HttpClient::buildRedirectRequest - Redirecting to: %s", location.c_str());

        Request redirectRequest;
        redirectRequest.setUrl(location);

        HttpMethod method = originalRequest.getMethod();
        std::string body = originalRequest.getBody();
        if (shouldConvertToGetOnRedirect(statusCode, method))
        {
            method = HttpMethod::GET;
            body.clear();
        }
        redirectRequest.setMethod(method);
        redirectRequest.setBody(body);

        const bool sameOrigin = isSameOrigin(originalRequest.getUrl(), location);
        for (const auto &header : originalRequest.getHeaders())
        {
            if (Utils::caseInsensitiveCompare(header.first, "Host"))
            {
                continue;
            }
            if (Utils::caseInsensitiveCompare(header.first, "Content-Length"))
            {
                continue;
            }
            if (!sameOrigin && Utils::caseInsensitiveCompare(header.first, "Authorization"))
            {
                continue;
            }
            if (method == HttpMethod::GET &&
                (Utils::caseInsensitiveCompare(header.first, "Content-Type") ||
                 Utils::caseInsensitiveCompare(header.first, "Content-Length")))
            {
                continue;
            }

            redirectRequest.addHeader(header.first, header.second);
        }

        return redirectRequest;
    }

    Result<HttpResult> HttpClient::executeRequest(const Request &request)
    {
        return executeRequest(request, ReadOptions());
    }

    Result<HttpResult> HttpClient::executeRequest(const Request &request, ReadOptions options)
    {
        if (isCancelled())
        {
            return cancelledResult();
        }

        auto modifiedRequest = request;
        m_auth->applyAuthentication(modifiedRequest);

        auto connectionResult = establishConnection(modifiedRequest);
        if (connectionResult.isError())
        {
            LOG_ERROR_S("HttpClient::executeRequest - Connection establishment failed");
            return Result<HttpResult>(connectionResult.error());
        }
        auto connection = connectionResult.value();

        if (!connection)
        {
            LOG_ERROR_S("HttpClient::executeRequest - Connection is null");
            return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Connection is null"));
        }

        auto validationResult = RequestValidator::validate(modifiedRequest, m_options);
        if (validationResult.isError())
        {
            LOG_ERROR_S("HttpClient::executeRequest - Request validation failed");
            return Result<HttpResult>(validationResult.error());
        }

        std::string requestStr = buildRequestString(modifiedRequest);
        LOG_DEBUG("HttpClient::executeRequest - Request string built. Length: %zu", requestStr.length());

        if (isCancelled())
        {
            return cancelledResult();
        }

        auto writeStart = std::chrono::steady_clock::now();
        if (connection->write(reinterpret_cast<const uint8_t *>(requestStr.c_str()), requestStr.length()) != requestStr.length())
        {
            auto writeDuration = std::chrono::steady_clock::now() - writeStart;
            if (writeDuration > m_timeouts.write)
            {
                LOG_ERROR_S("HttpClient::executeRequest - Write operation timed out");
                return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Write operation timed out"));
            }
            LOG_ERROR_S("HttpClient::executeRequest - Failed to send request");
            return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Failed to send request"));
        }
        LOG_DEBUG_S("HttpClient::executeRequest - Request sent successfully");

        auto responseResult = readResponse(connection.get(), modifiedRequest, options);
        if (responseResult.isError())
        {
            return responseResult;
        }

        auto httpResult = responseResult.value();
        processCookies(modifiedRequest, httpResult);
        return Result<HttpResult>(std::move(httpResult));
    }

    Result<HttpResult> HttpClient::sendWithRedirects(const Request &request)
    {
        return sendWithRedirects(request, ReadOptions(), 0);
    }

    Result<HttpResult> HttpClient::sendWithRedirects(const Request &request, ReadOptions options, int /*redirectCount*/)
    {
        LOG_DEBUG("HttpClient::sendWithRedirects called. Current request URL: %s", request.getUrl().c_str());

        Request currentRequest = request;
        for (int redirectCount = 0; redirectCount <= m_options.maxRedirects; ++redirectCount)
        {
            auto responseResult = executeRequest(currentRequest, options);
            if (responseResult.isError())
            {
                return responseResult;
            }

            auto httpResult = responseResult.value();
            LOG_DEBUG("HttpClient::sendWithRedirects - Received status code: %d", httpResult.statusCode);

            if (httpResult.statusCode >= 200 && httpResult.statusCode < 300)
            {
                LOG_DEBUG_S("HttpClient::sendWithRedirects - Successful response (200-299)");
                return Result<HttpResult>(std::move(httpResult));
            }

            if (httpResult.statusCode >= 300 && httpResult.statusCode < 400)
            {
                if (!m_options.followRedirects)
                {
                    LOG_DEBUG_S("HttpClient::sendWithRedirects - Redirect function is disabled");
                    return Result<HttpResult>(std::move(httpResult));
                }

                if (redirectCount >= m_options.maxRedirects)
                {
                    LOG_ERROR_S("HttpClient::sendWithRedirects - Too many redirects");
                    return Result<HttpResult>(ErrorInfo(ErrorCode::TooManyRedirects, "Too many redirects"));
                }

                auto location = findHeaderValue(httpResult.headers, "Location");
                if (location.empty())
                {
                    LOG_ERROR_S("HttpClient::sendWithRedirects - Redirect location not found");
                    return Result<HttpResult>(ErrorInfo(ErrorCode::InvalidResponse, "Redirect location not found"));
                }

                currentRequest = buildRedirectRequest(currentRequest, httpResult, httpResult.statusCode);
                continue;
            }

            LOG_DEBUG("HttpClient::sendWithRedirects - Unhandled status code: %d", httpResult.statusCode);
            return Result<HttpResult>(std::move(httpResult));
        }

        LOG_ERROR_S("HttpClient::sendWithRedirects - Too many redirects");
        return Result<HttpResult>(ErrorInfo(ErrorCode::TooManyRedirects, "Too many redirects"));
    }

    void HttpClient::applyConnectionTimeouts(Connection *connection)
    {
        if (connection != nullptr)
        {
            connection->setTimeouts(m_timeouts.connect, m_timeouts.read, m_timeouts.write);
        }
    }

    Result<std::shared_ptr<Connection>> HttpClient::establishConnection(const Request &request)
    {
        std::string host = Utils::extractHost(request.getUrl());
        int port = Utils::extractPort(request.getUrl());

        std::shared_ptr<Connection> connection;
        if (m_useMock)
        {
            connection = m_mockConnection;
        }
        else
        {
            connection = m_connectionPool->getConnection(host, port);
        }

        if (!connection)
        {
            return Result<std::shared_ptr<Connection>>(ErrorInfo(ErrorCode::NetworkError, "Failed to get connection from pool"));
        }

        if (!m_options.proxyUrl.empty())
        {
            return establishProxyConnection(connection, request);
        }
        else
        {
            return establishDirectConnection(connection, host, port);
        }
    }

    Result<std::shared_ptr<Connection>> HttpClient::establishDirectConnection(std::shared_ptr<Connection> connection, const std::string &host, int port)
    {
        applyConnectionTimeouts(connection.get());
        auto connectStart = std::chrono::steady_clock::now();
        if (!connection->connect(host, port))
        {
            auto connectDuration = std::chrono::steady_clock::now() - connectStart;
            if (connectDuration > m_timeouts.connect)
            {
                return Result<std::shared_ptr<Connection>>(ErrorInfo(ErrorCode::Timeout, "Connection timed out"));
            }
            return Result<std::shared_ptr<Connection>>(ErrorInfo(ErrorCode::NetworkError, "Failed to connect to " + host));
        }
        return Result<std::shared_ptr<Connection>>(connection);
    }

    Result<std::shared_ptr<Connection>> HttpClient::establishProxyConnection(std::shared_ptr<Connection> connection, const Request &request)
    {
        std::string proxyHost = Utils::extractHost(m_options.proxyUrl);
        int proxyPort = Utils::extractPort(m_options.proxyUrl);
        m_options.verifySsl = Utils::extractScheme(m_options.proxyUrl) == "https";

        applyConnectionTimeouts(connection.get());
        auto connectStart = std::chrono::steady_clock::now();
        if (!connection->connect(proxyHost, proxyPort))
        {
            auto connectDuration = std::chrono::steady_clock::now() - connectStart;
            if (connectDuration > m_timeouts.connect)
            {
                return Result<std::shared_ptr<Connection>>(ErrorInfo(ErrorCode::Timeout, "Proxy connection timed out"));
            }
            return Result<std::shared_ptr<Connection>>(ErrorInfo(ErrorCode::NetworkError, "Failed to proxy connect to " + proxyHost));
        }

        if (m_options.verifySsl)
        {
            // プロキシ経由でのSSL通信の場合、トンネルを確立する
            auto tunnelResult = establishProxyTunnel(connection, request, proxyHost, proxyPort);
            if (tunnelResult.isError())
            {
                return tunnelResult;
            }
        }
        return Result<std::shared_ptr<Connection>>(connection);
    }

    Result<std::shared_ptr<Connection>> HttpClient::establishProxyTunnel(std::shared_ptr<Connection> connection, const Request &request, const std::string &proxyHost, int proxyPort)
    {
        std::string connectRequestStr = "CONNECT " + proxyHost + ":" + std::to_string(proxyPort) + " HTTP/1.1\r\n";
        connectRequestStr += "Host: " + proxyHost + ":" + std::to_string(proxyPort) + "\r\n";
        connectRequestStr += "Connection: keep-alive\r\n";
        connectRequestStr += "\r\n";

        auto proxyValidationResult = RequestValidator::validate(request, m_options);
        if (proxyValidationResult.isError())
        {
            return Result<std::shared_ptr<Connection>>(proxyValidationResult.error());
        }

        auto writeStart = std::chrono::steady_clock::now();
        if (connection->write(reinterpret_cast<const uint8_t *>(connectRequestStr.c_str()), connectRequestStr.length()) != connectRequestStr.length())
        {
            auto writeDuration = std::chrono::steady_clock::now() - writeStart;
            if (writeDuration > m_timeouts.write)
            {
                return Result<std::shared_ptr<Connection>>(ErrorInfo(ErrorCode::Timeout, "Proxy write operation timed out"));
            }
            return Result<std::shared_ptr<Connection>>(ErrorInfo(ErrorCode::NetworkError, "Failed to send proxy request"));
        }

        auto responseResult = readResponse(connection.get(), request);
        if (responseResult.isError() || responseResult.value().statusCode != 200)
        {
            return Result<std::shared_ptr<Connection>>(ErrorInfo(ErrorCode::NetworkError, "Failed to establish proxy tunnel"));
        }
        return Result<std::shared_ptr<Connection>>(connection);
    }

    Result<HttpResult> HttpClient::readResponse(Connection *connection, const Request &request)
    {
        return readResponse(connection, request, ReadOptions());
    }

    Result<HttpResult> HttpClient::readResponse(Connection *connection, const Request &request, ReadOptions options)
    {
        if (!connection)
        {
            return Result<HttpResult>(ErrorInfo(ErrorCode::InvalidResponse, "Connection is null"));
        }

        HttpResult httpResult;
        auto readStart = std::chrono::steady_clock::now();

        try
        {
            const size_t bufferSize = HttpClient::DEFAULT_BUFFER_SIZE;
            std::unique_ptr<uint8_t[]> buffer(new uint8_t[bufferSize]);
            size_t totalBytesRead = 0;
            std::string responseStr;
            std::string bodyAccumulator;
            bool headersCompleted = false;
            size_t contentLength = 0;
            bool chunked = false;
            bool useConnectionClose = false;

            while (connection->connected())
            {
                if (isCancelled())
                {
                    return cancelledResult();
                }

                auto elapsed = std::chrono::steady_clock::now() - readStart;
                if (elapsed >= m_timeouts.read)
                {
                    LOG_ERROR("HttpClient::readResponse - Read timeout reached. Elapsed: %lld ms, Timeout: %lld ms",
                              static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()),
                              static_cast<long long>(m_timeouts.read.count()));
                    return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Read operation timed out while reading response"));
                }

                if (m_useMock && headersCompleted && !chunked && !useConnectionClose &&
                    (bodyAccumulator.size() + responseStr.size()) >= contentLength)
                {
                    break;
                }

                size_t bytesAvailable = connection->available();
                LOG_DEBUG("HttpClient::readResponse - Bytes available: %zu", bytesAvailable);

                if (bytesAvailable > 0)
                {
                    size_t bytesToRead = std::min(bytesAvailable, bufferSize);
                    int bytesRead = connection->read(buffer.get(), bytesToRead);
                    LOG_DEBUG("HttpClient::readResponse - Bytes read: %d", bytesRead);

                    if (bytesRead < 0)
                    {
                        LOG_ERROR_S("HttpClient::readResponse - Read error occurred");
                        return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Read error occurred"));
                    }

                    if (bytesRead > 0)
                    {
                        responseStr.append(reinterpret_cast<char *>(buffer.get()), bytesRead);
                        totalBytesRead += bytesRead;
                        LOG_DEBUG("HttpClient::readResponse - Total bytes read: %zu", totalBytesRead);

                        if (!headersCompleted)
                        {
                            size_t headerEnd = responseStr.find("\r\n\r\n");
                            if (headerEnd != std::string::npos)
                            {
                                headersCompleted = true;
                                std::string headers = responseStr.substr(0, headerEnd);
                                Utils::parseStatusLine(headers, httpResult);
                                Utils::parseHeaders(headers, httpResult);
                                contentLength = Utils::extractContentLength(httpResult.headers);
                                chunked = isChunkedEncoding(httpResult.headers);
                                const bool hasContentLengthHeader =
                                    !findHeaderValue(httpResult.headers, "Content-Length").empty();
                                useConnectionClose = !chunked && !hasContentLengthHeader;
                                responseStr = responseStr.substr(headerEnd + 4);

                                if (!chunked && hasContentLengthHeader && contentLength == 0)
                                {
                                    LOG_DEBUG_S("HttpClient::readResponse - Zero content-length response received");
                                    if (m_useMock)
                                    {
                                        auto mockConnection = static_cast<MockWiFiClientSecure *>(connection);
                                        mockConnection->moveToNextResponse();
                                    }
                                    break;
                                }

                                if (chunked)
                                {
                                    httpResult.body = responseStr;
                                    return handleChunkedResponse(connection, httpResult, 0, options);
                                }
                            }
                        }

                        if (headersCompleted && !chunked)
                        {
                            const size_t pendingBodySize = bodyAccumulator.size() + responseStr.size();

                            if (useConnectionClose)
                            {
                                if (!responseStr.empty())
                                {
                                    notifyBodyChunk(responseStr.c_str(), responseStr.size(), 0, options, bodyAccumulator);
                                    responseStr.clear();
                                }
                            }
                            else if (pendingBodySize >= contentLength)
                            {
                                if (!responseStr.empty())
                                {
                                    notifyBodyChunk(responseStr.c_str(), responseStr.size(), contentLength, options, bodyAccumulator);
                                    responseStr.clear();
                                }

                                LOG_DEBUG_S("HttpClient::readResponse - Complete response received");
                                if (m_useMock)
                                {
                                    auto mockConnection = static_cast<MockWiFiClientSecure *>(connection);
                                    mockConnection->moveToNextResponse();
                                }
                                break;
                            }
                            else if (!options.streaming && !responseStr.empty())
                            {
                                notifyBodyChunk(responseStr.c_str(), responseStr.size(), contentLength, options, bodyAccumulator);
                                responseStr.clear();
                            }
                        }
                    }
                }
                else if (headersCompleted && useConnectionClose && !connection->connected())
                {
                    break;
                }
                else
                {
                    if (headersCompleted && useConnectionClose && bytesAvailable == 0 && !connection->connected())
                    {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }

            LOG_DEBUG_S("HttpClient::readResponse - Loop exited");

            if (isCancelled())
            {
                return cancelledResult();
            }

            LOG_DEBUG("HttpClient::readResponse - Parsed status line: %d %s", httpResult.statusCode, httpResult.statusMessage.c_str());

            if (options.streaming)
            {
                httpResult.body.clear();
            }
            else
            {
                httpResult.body = std::move(bodyAccumulator);
                if (!responseStr.empty())
                {
                    httpResult.body.append(responseStr);
                }
            }

            LOG_DEBUG("HttpClient::readResponse - Body length: %zu", httpResult.body.length());
            return Result<HttpResult>(std::move(httpResult));
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("HttpClient::readResponse - Exception caught: %s", e.what());
            return Result<HttpResult>(ErrorInfo(ErrorCode::InvalidResponse, e.what()));
        }
    }

    Result<HttpResult> HttpClient::handleChunkedResponse(Connection *connection, HttpResult &result, size_t startingPos)
    {
        return handleChunkedResponse(connection, result, startingPos, ReadOptions());
    }

    Result<HttpResult> HttpClient::handleChunkedResponse(Connection *connection, HttpResult &result, size_t startingPos, ReadOptions options)
    {
        auto readStart = std::chrono::steady_clock::now();
        const size_t bufferSize = HttpClient::DEFAULT_BUFFER_SIZE;
        std::unique_ptr<uint8_t[]> buffer(new uint8_t[bufferSize]);
        size_t totalRead = 0;

        std::string chunkedData = result.body;
        result.body.clear();
        size_t currentPos = startingPos;
        std::string bodyAccumulator;

        auto readMoreChunkedData = [&]()
        {
            size_t bytesAvailable = connection->available();
            if (bytesAvailable == 0)
            {
                return 0;
            }
            size_t bytesToRead = std::min(bytesAvailable, bufferSize);
            return connection->read(buffer.get(), bytesToRead);
        };

        while (true)
        {
            if (isCancelled())
            {
                return cancelledResult();
            }

            if (std::chrono::steady_clock::now() - readStart > m_timeouts.read)
            {
                return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Read operation timed out while reading chunked response"));
            }

            size_t chunkSizeLineEnd = chunkedData.find("\r\n", currentPos);
            if (chunkSizeLineEnd == std::string::npos)
            {
                int bytesRead = readMoreChunkedData();
                if (bytesRead > 0)
                {
                    chunkedData.append(reinterpret_cast<char *>(buffer.get()), bytesRead);
                    chunkSizeLineEnd = chunkedData.find("\r\n", currentPos);
                }
                else
                {
                    return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Connection closed unexpectedly"));
                }
            }

            std::string chunkSizeLine = chunkedData.substr(currentPos, chunkSizeLineEnd - currentPos);
            auto semicolonPos = chunkSizeLine.find(';');
            if (semicolonPos != std::string::npos)
            {
                chunkSizeLine = chunkSizeLine.substr(0, semicolonPos);
            }
            currentPos = chunkSizeLineEnd + 2;

            char *endptr = nullptr;
            size_t chunkSize = std::strtoul(chunkSizeLine.c_str(), &endptr, 16);
            if (endptr == chunkSizeLine.c_str() || chunkSizeLine.empty())
            {
                return Result<HttpResult>(ErrorInfo(ErrorCode::InvalidResponse, "Invalid chunk size: " + chunkSizeLine));
            }

            if (chunkSize == 0)
            {
                if (currentPos + 2 <= chunkedData.size() && chunkedData.compare(currentPos, 2, "\r\n") == 0)
                {
                    currentPos += 2;
                }
                else
                {
                    std::string trailerLine;
                    while (true)
                    {
                        trailerLine = connection->readLine();
                        if (trailerLine.empty() || trailerLine == "\r\n")
                        {
                            break;
                        }
                        Utils::parseHeader(trailerLine, result);
                    }
                }
                break;
            }

            while (currentPos + chunkSize + 2 > chunkedData.size())
            {
                int bytesRead = readMoreChunkedData();
                if (bytesRead > 0)
                {
                    chunkedData.append(reinterpret_cast<char *>(buffer.get()), bytesRead);
                }
                else
                {
                    return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Connection closed unexpectedly"));
                }
            }

            const char *chunkData = chunkedData.data() + currentPos;
            notifyBodyChunk(chunkData, chunkSize, 0, options, bodyAccumulator);
            totalRead += chunkSize;
            currentPos += chunkSize + 2;
        }

        if (options.streaming)
        {
            result.body.clear();
        }
        else
        {
            result.body = std::move(bodyAccumulator);
        }

        if (m_useMock)
        {
            auto mockConnection = static_cast<MockWiFiClientSecure *>(connection);
            mockConnection->moveToNextResponse();
        }

        return Result<HttpResult>(std::move(result));
    }

    void HttpClient::setTimeouts(const Timeouts &timeouts)
    {
        m_timeouts = timeouts;
    }

    void HttpClient::setConnectionTimeout(std::chrono::milliseconds timeout)
    {
        m_timeouts.connect = timeout;
    }

    void HttpClient::setReadTimeout(std::chrono::milliseconds timeout)
    {
        m_timeouts.read = timeout;
    }

    void HttpClient::setWriteTimeout(std::chrono::milliseconds timeout)
    {
        m_timeouts.write = timeout;
    }

    void HttpClient::cancel(const std::string & /*requestId*/)
    {
        m_cancelled = true;
        if (m_connectionPool)
        {
            m_connectionPool->disconnectAll();
        }
        if (m_mockConnection)
        {
            m_mockConnection->disconnect();
        }
    }

    void HttpClient::enableCookies(bool enable)
    {
        m_cookiesEnabled = enable;
    }

    void HttpClient::setProgressCallback(std::function<void(size_t, size_t)> callback)
    {
        m_progressCallback = std::move(callback);
    }

    void HttpClient::setResponseBodyCallback(std::function<void(const char *, size_t)> callback)
    {
        m_responseBodyCallback = std::move(callback);
    }

    Result<HttpResult> HttpClient::sendStreaming(const Request &request, ChunkCallback chunkCallback)
    {
        if (!chunkCallback)
        {
            return Result<HttpResult>(ErrorInfo(ErrorCode::InvalidOption, "Chunk callback is required for streaming"));
        }

        if (!m_isInitialized)
        {
            return Result<HttpResult>(std::move(m_initializationError));
        }

        m_cancelled = false;
        ReadOptions options;
        options.streaming = true;
        options.chunkCallback = std::move(chunkCallback);
        return sendWithRetries(request, options, 0);
    }

    std::string HttpClient::buildRequestString(const Request &request)
    {
        // URL成分を一度だけ解析してキャッシュ
        const std::string &url = request.getUrl();
        const std::string scheme = Utils::extractScheme(url);
        const std::string host = Utils::extractHost(url);
        const int port = Utils::extractPort(url);
        const std::string path = Utils::extractPath(url);

        std::ostringstream oss;

        if (!m_options.proxyUrl.empty())
        {
            // プロキシ使用時はリクエストラインに完全なURLを含める
            oss << canaspad::httpMethodToString(request.getMethod()) << " "
                << scheme << "://" << host << ":" << port << path << " HTTP/1.1\r\n";
        }
        else
        {
            // プロキシ未使用時はリクエストラインにパスのみを含める
            oss << canaspad::httpMethodToString(request.getMethod()) << " " << path << " HTTP/1.1\r\n";
        }
        // Host: ヘッダーを追加 ホストとポートを含める
        oss << "Host: " << host << ":" << port << "\r\n";

        // User-Agentが指定されていない場合はデフォルトを追加
        bool hasUserAgent = false;
        for (const auto &header : request.getHeaders())
        {
            if (Utils::caseInsensitiveCompare(header.first, "User-Agent"))
            {
                hasUserAgent = true;
                break;
            }
        }
        if (!hasUserAgent)
        {
            oss << "User-Agent: HttpClient-ESP32-Lib/1.0.0\r\n";
        }

        const auto &multipartParts = request.getMultipartParts();

        // プロキシ認証
        if (!m_options.proxyUrl.empty())
        {
            std::string encodedAuth = Utils::extractProxyAuth(m_options.proxyUrl);
            if (!encodedAuth.empty())
            {
                oss << "Proxy-Authorization: Basic " << encodedAuth << "\r\n";
            }
        }

        if (!multipartParts.empty())
        {
            std::string boundary = Utils::generateBoundary();
            oss << "Content-Type: multipart/form-data; boundary=" << boundary << "\r\n";

            std::string body;
            for (const auto &part : multipartParts)
            {
                body += "--" + boundary + "\r\n";
                if (!part.filename.empty())
                {
                    body += "Content-Disposition: form-data; name=\"" + part.name +
                            "\"; filename=\"" + part.filename + "\"\r\n";
                    body += "Content-Type: " + part.contentType + "\r\n\r\n";
                }
                else
                {
                    body += "Content-Disposition: form-data; name=\"" + part.name + "\"\r\n\r\n";
                }
                body += part.content + "\r\n";
            }
            body += "--" + boundary + "--\r\n";

            oss << "Content-Length: " << body.length() << "\r\n\r\n";
            oss << body;
        }
        else
        {
            for (const auto &header : request.getHeaders())
            {
                // Hostヘッダーは自動的に追加されるため、ここではスキップします
                if (Utils::caseInsensitiveCompare(header.first, "Host"))
                {
                    continue;
                }
                oss << header.first << ": " << header.second << "\r\n";
            }

            if (m_cookiesEnabled)
            {
                auto cookies = m_connectionPool->getCookieJar()->getCookiesForUrl(request.getUrl());
                if (!cookies.empty())
                {
                    oss << "Cookie: " << Utils::joinStrings(cookies, "; ") << "\r\n";
                }
            }

            if (!request.getBody().empty())
            {
                oss << "Content-Length: " << request.getBody().length() << "\r\n";
            }
            oss << "\r\n";
            oss << request.getBody();
        }

        return oss.str();
    }
} // namespace canaspad