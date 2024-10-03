#include "HttpClient.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>
#include "../utils/Utils.h"
#include "WiFiSecureConnection.h"
#include "mock/MockWiFiClientSecure.h"
#include "RequestValidator.h"
#include <Arduino.h>

namespace canaspad
{

    HttpClient::HttpClient(const ClientOptions &options, bool useMock)
        : m_connectionPool(std::make_unique<ConnectionPool>(
              options,
              useMock ? std::shared_ptr<Connection>(new MockWiFiClientSecure(options))
                      : std::shared_ptr<Connection>(new WiFiSecureConnection()))),
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
            m_connectionPool = std::make_unique<ConnectionPool>(options);

            time_t now;
            time(&now);
            if (now < 3600 * 9)
            {
                m_isInitialized = false;
                m_initializationError = (ErrorInfo(
                    ErrorCode::TimeNotSet,
                    "System time is not set. Please synchronize with NTP server."));
            }
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
        Serial.println("HttpClient::send called");
        Serial.printf("Initial request URL: %s\n", request.getUrl().c_str());
        if (!m_isInitialized)
        {
            return Result<HttpResult>(std::move(m_initializationError));
        }

        return sendWithRetries(request);
    }

    Result<HttpResult> HttpClient::sendWithRetries(const Request &request, int retryCount)
    {
        Serial.printf("HttpClient::sendWithRetries called. Retry count: %d\n", retryCount);
        Serial.printf("Request URL: %s\n", request.getUrl().c_str());
        auto result = sendWithRedirects(request);

        if (result.isError())
        {
            const auto &error = result.error();
            Serial.printf("Error encountered: Code %d, Message: %s\n", static_cast<int>(error.code), error.message.c_str());

            // Timeout の場合もリトライ対象に含める
            if ((error.code == ErrorCode::NetworkError || error.code == ErrorCode::Timeout) &&
                retryCount < m_options.maxRetries)
            {
                Serial.println("Retrying request...");
                // リトライ前に遅延を追加
                std::this_thread::sleep_for(m_options.retryDelay);

                return sendWithRetries(request, retryCount + 1);
            }
        }
        return result;
    }

    Result<HttpResult> HttpClient::sendWithRedirects(const Request &request, int redirectCount)
    {
        Serial.printf("HttpClient::sendWithRedirects called. Redirect count: %d\n", redirectCount);
        Serial.printf("Current request URL: %s\n", request.getUrl().c_str());
        auto modifiedRequest = request;

        // 認証情報を適用
        m_auth->applyAuthentication(modifiedRequest);

        // 接続の確立
        auto connectionResult = establishConnection(modifiedRequest);
        if (connectionResult.isError())
        {
            Serial.println("HttpClient::sendWithRedirects - Connection establishment failed");
            return Result<HttpResult>(connectionResult.error());
        }
        auto connection = connectionResult.value();

        std::string requestStr = buildRequestString(modifiedRequest);
        Serial.printf("HttpClient::sendWithRedirects - Request string built. Length: %zu\n", requestStr.length());

        // 各種設定のバリデーション
        auto validationResult = RequestValidator::validate(modifiedRequest, m_options);
        if (validationResult.isError())
        {
            Serial.println("HttpClient::sendWithRedirects - Request validation failed");
            return Result<HttpResult>(validationResult.error());
        }

        auto writeStart = std::chrono::steady_clock::now();
        if (connection->write(reinterpret_cast<const uint8_t *>(requestStr.c_str()), requestStr.length()) != requestStr.length())
        {
            auto writeDuration = std::chrono::steady_clock::now() - writeStart;
            if (writeDuration > m_timeouts.write)
            {
                Serial.println("HttpClient::sendWithRedirects - Write operation timed out");
                return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Write operation timed out"));
            }
            Serial.println("HttpClient::sendWithRedirects - Failed to send request");
            return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Failed to send request"));
        }
        Serial.println("HttpClient::sendWithRedirects - Request sent successfully");

        auto responseResult = readResponse(connection.get(), modifiedRequest);
        Serial.println("HttpClient::sendWithRedirects - Response Result:");
        if (responseResult.isSuccess())
        {
            const auto &httpResult = responseResult.value();
            Serial.printf("Status Code: %d\n", httpResult.statusCode);
            Serial.printf("Status Message: %s\n", httpResult.statusMessage.c_str());
            Serial.printf("Body length: %zu\n", httpResult.body.length());
        }
        else
        {
            Serial.printf("Error: %s\n", responseResult.error().message.c_str());
        }

        if (responseResult.isError())
        {
            Serial.println("HttpClient::sendWithRedirects responseResult.isError() true");
            return responseResult;
        }

        auto httpResult = responseResult.value();

        // クッキー処理
        if (m_cookiesEnabled)
        {
            Serial.println("HttpClient::sendWithRedirects - Processing cookies");
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
            Serial.println("HttpClient::sendWithRedirects - Too many redirects");
            return Result<HttpResult>(ErrorInfo(ErrorCode::TooManyRedirects, "Too many redirects"));
        }

        Serial.printf("HttpClient::sendWithRedirects - Received status code: %d\n", httpResult.statusCode);

        // 200 OKのレスポンスを正常に処理
        if (httpResult.statusCode >= 200 && httpResult.statusCode < 300)
        {
            Serial.println("HttpClient::sendWithRedirects - Successful response (200-299)");
            return Result<HttpResult>(std::move(httpResult));
        }

        if (httpResult.statusCode >= 300 && httpResult.statusCode < 400)
        {
            if (m_options.followRedirects)
            {
                Serial.println("HttpClient::sendWithRedirects - Redirect detected (300-399)");

                auto location = Utils::extractHeaderValue(httpResult.headers, "Location");
                if (!location.empty())
                {
                    if (location.find("://") == std::string::npos)
                    {
                        std::string baseUrl = Utils::extractBaseUrl(request.getUrl());
                        location = baseUrl + location;
                    }

                    Serial.printf("HttpClient::sendWithRedirects - Redirecting to: %s\n", location.c_str());

                    Request redirectRequest;
                    redirectRequest.setUrl(location);
                    redirectRequest.setMethod(request.getMethod());
                    redirectRequest.setBody(request.getBody());

                    // 新しい接続を確立
                    auto redirectConnectionResult = establishConnection(redirectRequest);
                    if (redirectConnectionResult.isError())
                    {
                        Serial.println("HttpClient::sendWithRedirects - Failed to establish connection for redirect");
                        return Result<HttpResult>(redirectConnectionResult.error());
                    }
                    auto redirectConnection = redirectConnectionResult.value();

                    // 新しいリクエストを書き込む
                    std::string redirectRequestStr = buildRequestString(redirectRequest);
                    Serial.printf("HttpClient::sendWithRedirects - Redirect request string built. Length: %zu\n", redirectRequestStr.length());
                    Serial.printf("HttpClient::sendWithRedirects - Redirect request: %s\n", redirectRequestStr.c_str());
                    auto writeStart = std::chrono::steady_clock::now();
                    if (redirectConnection->write(reinterpret_cast<const uint8_t *>(redirectRequestStr.c_str()), redirectRequestStr.length()) != redirectRequestStr.length())
                    {
                        auto writeDuration = std::chrono::steady_clock::now() - writeStart;
                        if (writeDuration > m_timeouts.write)
                        {
                            Serial.println("HttpClient::sendWithRedirects - Write operation timed out for redirect");
                            return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Write operation timed out for redirect"));
                        }
                        Serial.println("HttpClient::sendWithRedirects - Failed to send redirect request");
                        return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Failed to send redirect request"));
                    }

                    Serial.println("HttpClient::sendWithRedirects - Redirect request sent successfully");

                    // リダイレクト先からのレスポンスを読み取る
                    auto redirectResponseResult = readResponse(redirectConnection.get(), redirectRequest);
                    if (redirectResponseResult.isError())
                    {
                        Serial.println("HttpClient::sendWithRedirects - Failed to read redirect response");
                        return redirectResponseResult;
                    }

                    // 成功レスポンス（200-299）の場合は、そのレスポンスを返す
                    if (redirectResponseResult.value().statusCode >= 200 && redirectResponseResult.value().statusCode < 300)
                    {
                        Serial.println("HttpClient::sendWithRedirects - Successful response after redirect");
                        return redirectResponseResult;
                    }

                    // リダイレクトの場合
                    if (redirectResponseResult.value().statusCode >= 300 && redirectResponseResult.value().statusCode < 400)
                    {
                        Serial.println("HttpClient::sendWithRedirects - Redirect after redirect");
                        // TODO redirectResponseResultの情報を使って再帰的に処理したい、しかしながら関数がうまく分割されていないのでリダイレクト処理#1に戻ることが出来ない。
                    }

                    // それ以外の場合は再帰的に処理
                    return sendWithRedirects(redirectRequest, redirectCount + 1);
                }
                else
                {
                    Serial.println("HttpClient::sendWithRedirects - Redirect location not found");
                    return Result<HttpResult>(ErrorInfo(ErrorCode::InvalidResponse, "Redirect location not found"));
                }
            }
            else
            {
                Serial.println("HttpClient::sendWithRedirects - Redirect function is disabled");
            }
        }

        Serial.printf("HttpClient::sendWithRedirects - Unhandled status code: %d\n", httpResult.statusCode);
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
        HttpResult httpResult;
        auto readStart = std::chrono::steady_clock::now();

        try
        {
            const size_t bufferSize = 4096;
            uint8_t buffer[bufferSize];
            size_t totalBytesRead = 0;
            std::string responseStr;
            bool headersCompleted = false;
            size_t contentLength = 0;

            while (connection->connected() && connection->available() > 0)
            {
                // タイムアウトチェックを追加
                if (std::chrono::steady_clock::now() - readStart >= m_timeouts.read)
                {
                    Serial.println("HttpClient::readResponse - Read timeout reached");
                    return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Read operation timed out while reading response"));
                }

                if (m_useMock && headersCompleted && responseStr.length() >= contentLength)
                {
                    break; // モックオブジェクトを使用している場合、ここでループを抜ける
                }

                size_t bytesAvailable = connection->available();
                Serial.printf("HttpClient::readResponse - Bytes available: %zu\n", bytesAvailable);

                if (bytesAvailable > 0)
                {
                    size_t bytesToRead = std::min(bytesAvailable, bufferSize);
                    int bytesRead = connection->read(buffer, bytesToRead);
                    Serial.printf("HttpClient::readResponse - Bytes read: %d\n", bytesRead);

                    if (bytesRead > 0)
                    {
                        responseStr.append(reinterpret_cast<char *>(buffer), bytesRead);
                        totalBytesRead += bytesRead;
                        Serial.printf("HttpClient::readResponse - Total bytes read: %zu\n", totalBytesRead);

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
                            Serial.println("HttpClient::readResponse - Complete response received");
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
                    delay(10);
                }
            }

            // loopから抜けたことをプリント
            Serial.println("HttpClient::readResponse - Loop exited");
            Serial.printf("HttpClient::readResponse - Parsed status line: %d %s\n", httpResult.statusCode, httpResult.statusMessage.c_str());

            httpResult.body = std::move(responseStr);

            // デバッグ出力（既存のコード）
            Serial.println("HttpClient::readResponse - Parsed HttpResult:");
            Serial.printf("Status Code: %d\n", httpResult.statusCode);
            Serial.printf("Status Message: %s\n", httpResult.statusMessage.c_str());
            Serial.println("Headers:");
            for (const auto &header : httpResult.headers)
            {
                Serial.printf("%s: %s\n", header.first.c_str(), header.second.c_str());
            }
            Serial.printf("Body length: %zu\n", httpResult.body.length());

            auto result = Result<HttpResult>(std::move(httpResult));

            Serial.println("HttpClient::readResponse - Result<HttpResult>:");
            if (result.isSuccess())
            {
                Serial.println("Result is success");
            }
            else
            {
                Serial.printf("Result is error: %s\n", result.error().message.c_str());
            }

            return result;
        }
        catch (const std::exception &e)
        {
            Serial.printf("HttpClient::readResponse - Exception caught: %s\n", e.what());
            return Result<HttpResult>(ErrorInfo(ErrorCode::InvalidResponse, e.what()));
        }
    }

    Result<HttpResult> HttpClient::handleChunkedResponse(Connection *connection, HttpResult &result, size_t startingPos)
    {
        auto readStart = std::chrono::steady_clock::now();
        const size_t bufferSize = 4096;
        uint8_t buffer[bufferSize];
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
                int bytesRead = connection->read(buffer, bytesToRead);
                if (bytesRead > 0)
                {
                    chunkedData.append(reinterpret_cast<char *>(buffer), bytesRead);
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
                int bytesRead = connection->read(buffer, bytesToRead);
                if (bytesRead > 0)
                {
                    chunkedData.append(reinterpret_cast<char *>(buffer), bytesRead);
                    chunkDataEnd = chunkedData.find("\r\n", currentPos);
                }
                else
                {
                    return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Connection closed unexpectedly"));
                }
            }

            // チャンクデータを追加
            result.body.append(chunkedData.substr(currentPos, chunkSize));
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
        std::ostringstream oss;
        if (!m_options.proxyUrl.empty())
        {
            // プロキシ使用時はリクエストラインに完全なURLを含める
            oss << canaspad::httpMethodToString(request.getMethod()) << " " << Utils::extractScheme(request.getUrl()) << "://" << Utils::extractHost(request.getUrl()) << ":" << Utils::extractPort(request.getUrl()) << Utils::extractPath(request.getUrl()) << " HTTP/1.1\r\n";
        }
        else
        {
            // プロキシ未使用時はリクエストラインにパスのみを含める
            oss << canaspad::httpMethodToString(request.getMethod()) << " " << Utils::extractPath(request.getUrl()) << " HTTP/1.1\r\n";
        }
        // Host: ヘッダーを追加 ホストとポートを含める
        oss << "Host: " << Utils::extractHost(request.getUrl()) << ":" << Utils::extractPort(request.getUrl()) << "\r\n";
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