#include "HttpClient.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>
#include "../utils/Utils.h"
#include "WiFiSecureConnection.h"
#include "mock/MockWiFiClientSecure.h"

namespace canaspad
{

    HttpClient::HttpClient(const ClientOptions &options, bool useMock)
        : m_connectionPool(std::make_unique<ConnectionPool>(
              options,
              useMock ? std::shared_ptr<Connection>(new MockWiFiClientSecure(options))
                      : std::shared_ptr<Connection>(new WiFiSecureConnection()))),
          m_auth(std::make_unique<Auth>(options)),
          m_proxy(std::make_unique<Proxy>(options)),
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
        if (!m_isInitialized)
        {
            return Result<HttpResult>(std::move(m_initializationError));
        }
        return sendWithRetries(request);
    }

    Result<HttpResult> HttpClient::sendWithRetries(const Request &request, int retryCount)
    {
        auto result = sendWithRedirects(request);
        if (result.isError())
        {
            const auto &error = result.error();
            if ((error.code == ErrorCode::NetworkError || error.code == ErrorCode::Timeout) && retryCount < m_options.maxRetries)
            {
                std::this_thread::sleep_for(m_options.retryDelay);
                return sendWithRetries(request, retryCount + 1);
            }
        }
        else if (result.value().statusCode >= 500 && retryCount < m_options.maxRetries)
        {
            std::this_thread::sleep_for(m_options.retryDelay);
            return sendWithRetries(request, retryCount + 1);
        }
        return result;
    }

    Result<HttpResult> HttpClient::sendWithRedirects(const Request &request, int redirectCount)
    {

        auto modifiedRequest = request;

        // 認証情報を適用
        m_auth->applyAuthentication(modifiedRequest);

        // プロキシ設定を適用
        m_proxy->applyProxySettings(modifiedRequest);

        std::string host = Utils::extractHost(modifiedRequest.getUrl());
        int port = Utils::extractPort(modifiedRequest.getUrl());

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
            return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Failed to get connection from pool"));
        }

        auto connectStart = std::chrono::steady_clock::now();
        if (!connection->connect(host, port))
        {
            auto connectDuration = std::chrono::steady_clock::now() - connectStart;
            if (connectDuration > m_timeouts.connect)
            {
                return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Connection timed out"));
            }
            return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Failed to connect to " + host));
        }

        // プロキシトンネルの確立 (Proxy クラスに移動)
        if (m_proxy->isEnabled() && !m_proxy->establishTunnel(connection.get(), request))
        {
            return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Failed to establish proxy tunnel"));
        }

        std::string requestStr = buildRequestString(modifiedRequest);
        auto writeStart = std::chrono::steady_clock::now();
        if (connection->write(reinterpret_cast<const uint8_t *>(requestStr.c_str()), requestStr.length()) != requestStr.length())
        {
            auto writeDuration = std::chrono::steady_clock::now() - writeStart;
            if (writeDuration > m_timeouts.write)
            {
                return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Write operation timed out"));
            }
            return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Failed to send request"));
        }

        auto responseResult = readResponse(connection.get(), modifiedRequest);
        if (responseResult.isError())
        {
            return responseResult;
        }

        auto httpResult = responseResult.value();

        // クッキー処理 (CookieJar クラスに移動)
        if (m_cookiesEnabled)
        {
            for (const auto &setCookieHeader : Utils::extractHeaders(httpResult.headers, "Set-Cookie"))
            {
                Cookie cookie;
                Utils::parseCookie(setCookieHeader, cookie, modifiedRequest.getUrl());
                httpResult.cookies.push_back(cookie);
                m_connectionPool->getCookieJar()->setCookie(modifiedRequest.getUrl(), setCookieHeader);
            }
        }

        // リダイレクト処理
        if (m_options.followRedirects && redirectCount < m_options.maxRedirects)
        {
            if (httpResult.statusCode >= 300 && httpResult.statusCode < 400)
            {
                auto location = Utils::extractHeaderValue(httpResult.headers, "Location");
                if (!location.empty())
                {
                    if (location.find("://") == std::string::npos)
                    {
                        std::string baseUrl = Utils::extractBaseUrl(modifiedRequest.getUrl());
                        location = baseUrl + location;
                    }

                    Request redirectRequest = modifiedRequest;
                    redirectRequest.setUrl(location);
                    return sendWithRedirects(redirectRequest, redirectCount + 1);
                }
            }
        }

        return httpResult;
    }

    Result<HttpResult> HttpClient::readResponse(Connection *connection, const Request &request)
    {
        HttpResult httpResult;
        auto readStart = std::chrono::steady_clock::now();

        try
        {
            std::string statusLine = connection->readLine();
            if (std::chrono::steady_clock::now() - readStart > m_timeouts.read)
            {
                return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Read operation timed out while reading status line"));
            }
            Utils::parseStatusLine(statusLine, httpResult);

            std::string headerLine;
            while ((headerLine = connection->readLine()) != "\r\n")
            {
                if (std::chrono::steady_clock::now() - readStart > m_timeouts.read)
                {
                    return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Read operation timed out while reading headers"));
                }
                Utils::parseHeader(headerLine, httpResult);
            }

            auto transferEncoding = Utils::extractHeaderValue(httpResult.headers, "Transfer-Encoding");
            if (transferEncoding == "chunked")
            {
                return handleChunkedResponse(connection, httpResult);
            }
            else
            {
                std::vector<char> buffer(8192);
                size_t totalRead = 0;
                size_t contentLength = Utils::extractContentLength(httpResult.headers);

                auto startTime = std::chrono::steady_clock::now();
                while (std::chrono::steady_clock::now() - startTime < m_timeouts.read)
                {
                    size_t bytesToRead = contentLength > 0 ? std::min(buffer.size(), contentLength - totalRead) : buffer.size();
                    size_t bytesRead = connection->read(reinterpret_cast<uint8_t *>(buffer.data()), bytesToRead);
                    if (bytesRead == 0)
                    {
                        break;
                    }

                    if (m_responseBodyCallback)
                    {
                        m_responseBodyCallback(buffer.data(), bytesRead);
                    }
                    else
                    {
                        httpResult.body.append(buffer.data(), bytesRead);
                    }

                    totalRead += bytesRead;

                    if (m_progressCallback)
                    {
                        m_progressCallback(totalRead, contentLength);
                    }

                    if (contentLength > 0 && totalRead >= contentLength)
                    {
                        break;
                    }
                }

                if (std::chrono::steady_clock::now() - startTime >= m_timeouts.read)
                {
                    return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Read operation timed out while reading body"));
                }
            }

            return Result<HttpResult>(std::move(httpResult));
        }
        catch (const std::exception &e)
        {
            return Result<HttpResult>(ErrorInfo(ErrorCode::InvalidResponse, e.what()));
        }
    }

    Result<HttpResult> HttpClient::handleChunkedResponse(Connection *connection, HttpResult &result)
    {
        auto readStart = std::chrono::steady_clock::now();
        std::vector<char> buffer(8192);
        size_t totalRead = 0;
        while (true)
        {
            if (std::chrono::steady_clock::now() - readStart > m_timeouts.read)
            {
                return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Read operation timed out while reading chunked response"));
            }

            std::string chunkSizeLine = connection->readLine();

            char *endptr;
            size_t chunkSize = std::strtoul(chunkSizeLine.c_str(), &endptr, 16);
            if (*endptr != '\0' || chunkSizeLine.empty())
            {
                return Result<HttpResult>(ErrorInfo(ErrorCode::InvalidResponse, "Invalid chunk size: " + chunkSizeLine));
            }

            if (chunkSize == 0)
            {
                break;
            }

            size_t chunkRead = 0;
            while (chunkRead < chunkSize)
            {
                if (std::chrono::steady_clock::now() - readStart > m_timeouts.read)
                {
                    return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Read operation timed out while reading chunk"));
                }

                size_t bytesToRead = std::min(buffer.size(), chunkSize - chunkRead);
                size_t bytesRead = connection->read(reinterpret_cast<uint8_t *>(buffer.data()), bytesToRead);

                if (bytesRead == 0)
                {
                    return Result<HttpResult>(ErrorInfo(ErrorCode::NetworkError, "Connection closed unexpectedly"));
                }

                if (m_responseBodyCallback)
                {
                    m_responseBodyCallback(buffer.data(), bytesRead);
                }
                else
                {
                    result.body.append(buffer.data(), bytesRead);
                }

                chunkRead += bytesRead;
                totalRead += bytesRead;

                if (m_progressCallback)
                {
                    m_progressCallback(totalRead, 0);
                }
            }

            connection->readLine();

            std::string trailerLine;
            while ((trailerLine = connection->readLine()) != "\r\n")
            {
                if (std::chrono::steady_clock::now() - readStart > m_timeouts.read)
                {
                    return Result<HttpResult>(ErrorInfo(ErrorCode::Timeout, "Read operation timed out while reading trailer headers"));
                }
                Utils::parseHeader(trailerLine, result);
            }
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
        oss << Utils::methodToString(request.getMethod()) << " " << Utils::extractPath(request.getUrl()) << " HTTP/1.1\r\n";
        oss << "Host: " << Utils::extractHost(request.getUrl()) << "\r\n";
        const auto &multipartFormData = request.getMultipartFormData();
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

            oss << "Content-Length: " << request.getBody().length() << "\r\n";
            oss << "\r\n";
            oss << request.getBody();
        }

        return oss.str();
    }

} // namespace canaspad