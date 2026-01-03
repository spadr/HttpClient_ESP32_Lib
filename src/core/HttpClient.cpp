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

        return sendWithRetries(request);
    }

    Result<HttpResult> HttpClient::sendWithRetries(const Request &request, int retryCount)
    {
        LOG_DEBUG("HttpClient::sendWithRetries called. Retry count: %d", retryCount);
        LOG_DEBUG("Request URL: %s", request.getUrl().c_str());
        auto result = sendWithRedirects(request);

        if (result.isError())
        {
            const auto &error = result.error();
            LOG_ERROR("Error encountered: Code %d, Message: %s", static_cast<int>(error.code), error.message.c_str());

            // Timeout の場合もリトライ対象に含める
            if ((error.code == ErrorCode::NetworkError || error.code == ErrorCode::Timeout) &&
                retryCount < m_options.maxRetries)
            {
                LOG_INFO_S("Retrying request...");
                // リトライ前に遅延を追加
                std::this_thread::sleep_for(m_options.retryDelay);

                return sendWithRetries(request, retryCount + 1);
            }
        }
        return result;
    }

    Result<HttpResult> HttpClient::sendWithRedirects(const Request &request, int redirectCount)
    {
        LOG_DEBUG("HttpClient::sendWithRedirects called. Redirect count: %d", redirectCount);
        LOG_DEBUG("Current request URL: %s", request.getUrl().c_str());
        auto modifiedRequest = request;

        // 認証情報を適用
        m_auth->applyAuthentication(modifiedRequest);

        // 接続の確立
        auto connectionResult = establishConnection(modifiedRequest);
        if (connectionResult.isError())
        {
            LOG_ERROR_S("HttpClient::sendWithRedirects - Connection establishment failed");
            return Result<HttpResult>(connectionResult.error());
        }
        auto connection = connectionResult.value();

        // Connection null check
        if (!connection)
        {
            LOG_ERROR_S("HttpClient::sendWithRedirects - Connection is null");
            return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Connection is null"));
        }

        std::string requestStr = buildRequestString(modifiedRequest);
        LOG_DEBUG("HttpClient::sendWithRedirects - Request string built. Length: %zu", requestStr.length());

        // 各種設定のバリデーション
        auto validationResult = RequestValidator::validate(modifiedRequest, m_options);
        if (validationResult.isError())
        {
            LOG_ERROR_S("HttpClient::sendWithRedirects - Request validation failed");
            return Result<HttpResult>(validationResult.error());
        }

        auto writeStart = std::chrono::steady_clock::now();
        if (connection->write(reinterpret_cast<const uint8_t *>(requestStr.c_str()), requestStr.length()) != requestStr.length())
        {
            auto writeDuration = std::chrono::steady_clock::now() - writeStart;
            if (writeDuration > m_timeouts.write)
            {
                LOG_ERROR_S("HttpClient::sendWithRedirects - Write operation timed out");
                return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Write operation timed out"));
            }
            LOG_ERROR_S("HttpClient::sendWithRedirects - Failed to send request");
            return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Failed to send request"));
        }
        LOG_DEBUG_S("HttpClient::sendWithRedirects - Request sent successfully");

        auto responseResult = readResponse(connection.get(), modifiedRequest);
        LOG_DEBUG_S("HttpClient::sendWithRedirects - Response Result:");
        if (responseResult.isSuccess())
        {
            const auto &httpResult = responseResult.value();
            LOG_DEBUG("Status Code: %d", httpResult.statusCode);
            LOG_DEBUG("Status Message: %s", httpResult.statusMessage.c_str());
            LOG_DEBUG("Body length: %zu", httpResult.body.length());
        }
        else
        {
            LOG_ERROR("Error: %s", responseResult.error().message.c_str());
        }

        if (responseResult.isError())
        {
            LOG_DEBUG_S("HttpClient::sendWithRedirects responseResult.isError() true");
            return responseResult;
        }

        auto httpResult = responseResult.value();

        // クッキー処理
        if (m_cookiesEnabled)
        {
            LOG_DEBUG_S("HttpClient::sendWithRedirects - Processing cookies");
            for (const auto &setCookieHeader : Utils::extractHeaders(httpResult.headers, "Set-Cookie"))
            {
                Cookie cookie;
                Utils::parseCookie(setCookieHeader, cookie, modifiedRequest.getUrl());
                httpResult.cookies.push_back(cookie);
                m_connectionPool->getCookieJar()->setCookie(modifiedRequest.getUrl(), setCookieHeader);
            }
        }

        // リダイレクト処理#1
        // リダイレクト回数が最大を超えているかを確認
        if (redirectCount >= m_options.maxRedirects)
        {
            LOG_ERROR_S("HttpClient::sendWithRedirects - Too many redirects");
            return Result<HttpResult>(ErrorInfo(ErrorCode::TooManyRedirects, "Too many redirects"));
        }

        LOG_DEBUG("HttpClient::sendWithRedirects - Received status code: %d", httpResult.statusCode);

        // 200 OKのレスポンスを正常に処理
        if (httpResult.statusCode >= 200 && httpResult.statusCode < 300)
        {
            LOG_DEBUG_S("HttpClient::sendWithRedirects - Successful response (200-299)");
            return Result<HttpResult>(std::move(httpResult));
        }

        if (httpResult.statusCode >= 300 && httpResult.statusCode < 400)
        {
            if (m_options.followRedirects)
            {
                LOG_DEBUG_S("HttpClient::sendWithRedirects - Redirect detected (300-399)");

                auto location = Utils::extractHeaderValue(httpResult.headers, "Location");
                if (!location.empty())
                {
                    if (location.find("://") == std::string::npos)
                    {
                        std::string baseUrl = Utils::extractBaseUrl(request.getUrl());
                        location = baseUrl + location;
                    }

                    LOG_DEBUG("HttpClient::sendWithRedirects - Redirecting to: %s", location.c_str());

                    Request redirectRequest;
                    redirectRequest.setUrl(location);
                    redirectRequest.setMethod(request.getMethod());
                    redirectRequest.setBody(request.getBody());

                    // 元のリクエストのヘッダーを引き継ぐ
                    // ただし、HostやContent-Lengthなどの特定のヘッダーは除外する必要がある場合があるが、
                    // 現状のRequestクラスの仕様では上書きされるか、HttpClient::buildRequestStringで生成されるため
                    // そのままコピーして問題ないもの（認証トークンなど）を優先する。
                    for (const auto &header : request.getHeaders())
                    {
                        // HostヘッダーはURLから自動生成されるためコピーしない（buildRequestStringで処理）
                        if (Utils::caseInsensitiveCompare(header.first, "Host"))
                            continue;
                        // Content-Lengthも自動計算されるためコピーしない
                        if (Utils::caseInsensitiveCompare(header.first, "Content-Length"))
                            continue;

                        redirectRequest.addHeader(header.first, header.second);
                    }

                    // 新しい接続を確立
                    auto redirectConnectionResult = establishConnection(redirectRequest);
                    if (redirectConnectionResult.isError())
                    {
                        LOG_ERROR_S("HttpClient::sendWithRedirects - Failed to establish connection for redirect");
                        return Result<HttpResult>(redirectConnectionResult.error());
                    }
                    auto redirectConnection = redirectConnectionResult.value();

                    // 新しいリクエストを書き込む
                    std::string redirectRequestStr = buildRequestString(redirectRequest);
                    LOG_DEBUG("HttpClient::sendWithRedirects - Redirect request string built. Length: %zu", redirectRequestStr.length());
                    LOG_DEBUG("HttpClient::sendWithRedirects - Redirect request: %s", redirectRequestStr.c_str());
                    auto writeStart = std::chrono::steady_clock::now();
                    if (redirectConnection->write(reinterpret_cast<const uint8_t *>(redirectRequestStr.c_str()), redirectRequestStr.length()) != redirectRequestStr.length())
                    {
                        auto writeDuration = std::chrono::steady_clock::now() - writeStart;
                        if (writeDuration > m_timeouts.write)
                        {
                            LOG_ERROR_S("HttpClient::sendWithRedirects - Write operation timed out for redirect");
                            return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Write operation timed out for redirect"));
                        }
                        LOG_ERROR_S("HttpClient::sendWithRedirects - Failed to send redirect request");
                        return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Failed to send redirect request"));
                    }

                    LOG_DEBUG_S("HttpClient::sendWithRedirects - Redirect request sent successfully");

                    // リダイレクト先からのレスポンスを読み取る
                    auto redirectResponseResult = readResponse(redirectConnection.get(), redirectRequest);
                    if (redirectResponseResult.isError())
                    {
                        LOG_ERROR_S("HttpClient::sendWithRedirects - Failed to read redirect response");
                        return redirectResponseResult;
                    }

                    // 成功レスポンス（200-299）の場合は、そのレスポンスを返す
                    if (redirectResponseResult.value().statusCode >= 200 && redirectResponseResult.value().statusCode < 300)
                    {
                        LOG_DEBUG_S("HttpClient::sendWithRedirects - Successful response after redirect");
                        return redirectResponseResult;
                    }

                    // リダイレクトの場合
                    if (redirectResponseResult.value().statusCode >= 300 && redirectResponseResult.value().statusCode < 400)
                    {
                        LOG_DEBUG_S("HttpClient::sendWithRedirects - Redirect after redirect");
                        // TODO redirectResponseResultの情報を使って再帰的に処理したい、しかしながら関数がうまく分割されていないのでリダイレクト処理#1に戻ることが出来ない。
                    }

                    // それ以外の場合は再帰的に処理
                    return sendWithRedirects(redirectRequest, redirectCount + 1);
                }
                else
                {
                    LOG_ERROR_S("HttpClient::sendWithRedirects - Redirect location not found");
                    return Result<HttpResult>(ErrorInfo(ErrorCode::InvalidResponse, "Redirect location not found"));
                }
            }
            else
            {
                LOG_DEBUG_S("HttpClient::sendWithRedirects - Redirect function is disabled");
            }
        }

        LOG_DEBUG("HttpClient::sendWithRedirects - Unhandled status code: %d", httpResult.statusCode);
        return Result<HttpResult>(std::move(httpResult));
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
        if (!connection)
        {
            return Result<HttpResult>(ErrorInfo(ErrorCode::InvalidResponse, "Connection is null"));
        }

        HttpResult httpResult;
        auto readStart = std::chrono::steady_clock::now();

        try
        {
            // Use heap allocation instead of stack to avoid stack overflow
            const size_t bufferSize = HttpClient::DEFAULT_BUFFER_SIZE;
            std::unique_ptr<uint8_t[]> buffer(new uint8_t[bufferSize]);
            size_t totalBytesRead = 0;
            std::string responseStr;
            bool headersCompleted = false;
            size_t contentLength = 0;

            while (connection->connected())
            {
                // タイムアウトチェックを追加
                auto elapsed = std::chrono::steady_clock::now() - readStart;
                if (elapsed >= m_timeouts.read)
                {
                    LOG_ERROR("HttpClient::readResponse - Read timeout reached. Elapsed: %lld ms, Timeout: %lld ms",
                              static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()),
                              static_cast<long long>(m_timeouts.read.count()));
                    return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Read operation timed out while reading response"));
                }

                if (m_useMock && headersCompleted && responseStr.length() >= contentLength)
                {
                    break; // モックオブジェクトを使用している場合、ここでループを抜ける
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
                                responseStr = responseStr.substr(headerEnd + 4);
                            }
                        }

                        // レスポンスの終わりを検出する処理を追加
                        if (headersCompleted && responseStr.length() >= contentLength)
                        {
                            LOG_DEBUG_S("HttpClient::readResponse - Complete response received");
                            if (m_useMock)
                            {
                                auto mockConnection = static_cast<MockWiFiClientSecure *>(connection);
                                mockConnection->moveToNextResponse();
                            }
                            break;
                        }
                    }
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }

            // loopから抜けたことをプリント
            LOG_DEBUG_S("HttpClient::readResponse - Loop exited");
            LOG_DEBUG("HttpClient::readResponse - Parsed status line: %d %s", httpResult.statusCode, httpResult.statusMessage.c_str());

            httpResult.body = std::move(responseStr);

            // デバッグ出力（既存のコード）
            LOG_DEBUG_S("HttpClient::readResponse - Parsed HttpResult:");
            LOG_DEBUG("Status Code: %d", httpResult.statusCode);
            LOG_DEBUG("Status Message: %s", httpResult.statusMessage.c_str());
            LOG_DEBUG_S("Headers:");
            for (const auto &header : httpResult.headers)
            {
                LOG_DEBUG("%s: %s", header.first.c_str(), header.second.c_str());
            }
            LOG_DEBUG("Body length: %zu", httpResult.body.length());

            auto result = Result<HttpResult>(std::move(httpResult));

            LOG_DEBUG_S("HttpClient::readResponse - Result<HttpResult>:");
            if (result.isSuccess())
            {
                LOG_DEBUG_S("Result is success");
            }
            else
            {
                LOG_ERROR("Result is error: %s", result.error().message.c_str());
            }

            return result;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("HttpClient::readResponse - Exception caught: %s", e.what());
            return Result<HttpResult>(ErrorInfo(ErrorCode::InvalidResponse, e.what()));
        }
    }

    Result<HttpResult> HttpClient::handleChunkedResponse(Connection *connection, HttpResult &result, size_t startingPos)
    {
        auto readStart = std::chrono::steady_clock::now();
        const size_t bufferSize = 4096;
        std::unique_ptr<uint8_t[]> buffer(new uint8_t[bufferSize]);
        size_t totalRead = 0;

        // startingPos から読み込みを開始
        std::string chunkedData = result.body; // 既存のデータ
        size_t currentPos = startingPos;

        while (true)
        {
            if (std::chrono::steady_clock::now() - readStart > m_timeouts.read)
            {
                return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Read operation timed out while reading chunked response"));
            }

            // チャンクサイズ行を見つける
            size_t chunkSizeLineEnd = chunkedData.find("\r\n", currentPos);
            if (chunkSizeLineEnd == std::string::npos)
            {
                // データが足りない場合は、さらに読み込む
                size_t bytesToRead = std::min(bufferSize, chunkedData.capacity() - chunkedData.size());
                int bytesRead = connection->read(buffer.get(), bytesToRead);
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
            currentPos = chunkSizeLineEnd + 2; // 次のチャンクの開始位置に更新

            char *endptr;
            size_t chunkSize = std::strtoul(chunkSizeLine.c_str(), &endptr, 16);
            if (*endptr != '\0' || chunkSizeLine.empty())
            {
                return Result<HttpResult>(ErrorInfo(ErrorCode::InvalidResponse, "Invalid chunk size: " + chunkSizeLine));
            }

            if (chunkSize == 0)
            {
                // チャンクの終わり
                break;
            }

            // チャンクデータを見つける
            size_t chunkDataEnd = chunkedData.find("\r\n", currentPos);
            while (chunkDataEnd == std::string::npos)
            {
                // データが足りない場合は、さらに読み込む
                size_t bytesToRead = std::min(bufferSize, chunkedData.capacity() - chunkedData.size());
                int bytesRead = connection->read(buffer.get(), bytesToRead);
                if (bytesRead > 0)
                {
                    chunkedData.append(reinterpret_cast<char *>(buffer.get()), bytesRead);
                    chunkDataEnd = chunkedData.find("\r\n", currentPos);
                }
                else
                {
                    return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Connection closed unexpectedly"));
                }
            }

            // チャンクデータを追加
            // 既存のbodyに直接追加することで、一時変数のコピーを回避
            result.body.append(chunkedData, currentPos, chunkSize);
            totalRead += chunkSize;
            currentPos = chunkDataEnd + 2; // 次のチャンクの開始位置に更新
        }

        // トレーラーヘッダーを読み込む
        std::string trailerLine;
        while ((trailerLine = connection->readLine()) != "\r\n")
        {
            if (std::chrono::steady_clock::now() - readStart > m_timeouts.read)
            {
                return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Read operation timed out while reading trailer headers"));
            }
            Utils::parseHeader(trailerLine, result);
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

    void HttpClient::cancel(const std::string &requestId)
    {
        // 実装は基盤となるネットワーク層に依存します
        // 現在は、単に切断します
        m_connectionPool->disconnectAll();
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
        // ストリーミング送信は未実装
        return Result<HttpResult>(ErrorInfo(ErrorCode::UnsupportedOperation, "Streaming is not yet supported."));
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

        const auto &multipartFormData = request.getMultipartFormData();

        // プロキシ認証
        if (!m_options.proxyUrl.empty())
        {
            std::string encodedAuth = Utils::extractProxyAuth(m_options.proxyUrl);
            if (!encodedAuth.empty())
            {
                oss << "Proxy-Authorization: Basic " << encodedAuth << "\r\n";
            }
        }

        if (!multipartFormData.empty())
        {
            std::string boundary = Utils::generateBoundary();
            oss << "Content-Type: multipart/form-data; boundary=" << boundary << "\r\n";

            std::string body;
            for (const auto &[key, value] : multipartFormData)
            {
                body += "--" + boundary + "\r\n";
                body += "Content-Disposition: form-data; name=\"" + key + "\"\r\n\r\n";
                body += value + "\r\n";
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