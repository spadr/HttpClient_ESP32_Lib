#include "HttpClient.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>

namespace http
{

    HttpClient::HttpClient(const ClientOptions &options)
        : m_wifiClient(std::make_shared<WiFiSecureClient>()), // shared_ptr を作成
          m_networkLayer(m_wifiClient.get()),                 // raw pointer を代入
          m_cookieJar(std::make_unique<CookieJar>()),
          m_connectionTimeout(std::chrono::seconds(30)),
          m_readTimeout(std::chrono::seconds(30)),
          m_writeTimeout(std::chrono::seconds(30)),
          m_cookiesEnabled(false),
          m_options(options),
          m_logger(options.logger),
          m_connectionPool(std::make_unique<ConnectionPool>())
    {
        // ClientOptions から証明書情報を WiFiSecureClient に設定
        if (options.verifySsl)
        {
            if (!options.rootCA.empty())
            {
                m_wifiClient->setCACert(options.rootCA.c_str());
            }
            if (!options.clientCert.empty() && !options.clientPrivateKey.empty())
            {
                m_wifiClient->setClientCert(options.clientCert.c_str(), options.clientPrivateKey.c_str());
            }
        }
    }

    HttpClient::~HttpClient() = default;

    std::future<std::unique_ptr<Response>> HttpClient::send(const Request &request)
    {
        return std::async(std::launch::async, [this, request]()
                          { return sendWithRetries(request); });
    }

    std::unique_ptr<Response> HttpClient::sendWithRetries(const Request &request, int retryCount)
    {
        try
        {
            auto response = sendWithRedirects(request);
            if (response->getStatusCode() >= 500 && retryCount < m_options.maxRetries)
            {
                std::this_thread::sleep_for(m_options.retryDelay);
                return sendWithRetries(request, retryCount + 1);
            }
            return response;
        }
        catch (const HttpError &e)
        {
            if (e.getErrorCode() == HttpError::ErrorCode::NetworkError && retryCount < m_options.maxRetries)
            {
                std::this_thread::sleep_for(m_options.retryDelay);
                return sendWithRetries(request, retryCount + 1);
            }
            throw;
        }
    }

    std::unique_ptr<Response> HttpClient::sendWithRedirects(const Request &request, int redirectCount)
    {
        Serial.println("sendWithRedirects");
        logRequest(request);

        auto modifiedRequest = request;
        applyAuthentication(modifiedRequest);

        std::string host = extractHost(modifiedRequest.getUrl());
        int port = extractPort(modifiedRequest.getUrl());

        if (m_proxySettings)
        {
            host = m_proxySettings->host;
            port = m_proxySettings->port;
        }

        // ConnectionPool から接続を取得する際に、設定済みの WiFiSecureClient を生成するラムダ式を渡す
        auto connection = m_connectionPool->getConnection(
            host + ":" + std::to_string(port),
            [this]()
            {
                return m_wifiClient; // shared_ptr を直接返す
            });

        if (connection == nullptr)
        {
            throwNetworkError("Failed to get connection from pool");
        }

        // m_networkLayer->setVerifySsl(m_options.verifySsl); // コンストラクタで設定済み

        if (!connection->connect(host, port))
        {
            throwNetworkError("Failed to connect to " + host);
        }

        Serial.println("Connected to host");

        if (!connection->isConnected() && !connection->connect(host, port))
        {
            throwNetworkError("Failed to connect to " + host);
        }

        Serial.println("Connected to host");

        if (m_proxySettings)
        {
            // CONNECT メソッドを使用してプロキシトンネルを確立
            std::string connectRequest = "CONNECT " + extractHost(request.getUrl()) + ":" + std::to_string(extractPort(request.getUrl())) + " HTTP/1.1\r\n";
            connectRequest += "Host: " + extractHost(request.getUrl()) + ":" + std::to_string(extractPort(request.getUrl())) + "\r\n";

            if (!m_proxySettings->username.empty() && !m_proxySettings->password.empty())
            {
                std::string auth = m_proxySettings->username + ":" + m_proxySettings->password;
                connectRequest += "Proxy-Authorization: Basic " + base64Encode(auth) + "\r\n";
            }

            connectRequest += "\r\n";

            connection->write(reinterpret_cast<const uint8_t *>(connectRequest.c_str()), connectRequest.length());

            // プロキシからの応答を読み取り、200 OK を確認
            std::string response = connection->readLine();
            if (response.find("200") == std::string::npos)
            {
                throwNetworkError("Failed to establish proxy tunnel: " + response);
            }

            // 残りのヘッダーを読み飛ばす
            while (connection->readLine() != "\r\n")
            {
            }
        }

        Serial.println("Connected to proxy");
        std::string requestStr = buildRequestString(modifiedRequest);
        if (connection->write(reinterpret_cast<const uint8_t *>(requestStr.c_str()), requestStr.length()) != requestStr.length())
        {
            throwNetworkError("Failed to send request");
        }
        Serial.println("Request sent");
        auto response = readResponse(connection.get(), modifiedRequest);
        Serial.println("Response received");
        logResponse(*response);

        if (m_cookiesEnabled)
        {
            auto setCookieHeaders = response->getHeaders().equal_range("Set-Cookie");
            for (auto it = setCookieHeaders.first; it != setCookieHeaders.second; ++it)
            {
                m_cookieJar->setCookie(modifiedRequest.getUrl(), it->second);
            }
        }
        Serial.println("Cookies set");

        if (m_options.followRedirects && redirectCount < m_options.maxRedirects)
        {
            int statusCode = response->getStatusCode();
            if (statusCode >= 300 && statusCode < 400)
            {
                auto it = response->getHeaders().find("Location");
                if (it != response->getHeaders().end())
                {
                    Request redirectRequest = modifiedRequest;
                    redirectRequest.setUrl(it->second);
                    return sendWithRedirects(redirectRequest, redirectCount + 1);
                }
            }
        }
        Serial.println("Redirects handled");
        m_connectionPool->releaseConnection(host + ":" + std::to_string(port), connection);
        Serial.println("Connection released");
        return response;
    }

    std::unique_ptr<Response> HttpClient::sendStreamingWithRedirects(const Request &request, ChunkCallback chunkCallback, int redirectCount)
    {
        logRequest(request);

        auto modifiedRequest = request;
        applyAuthentication(modifiedRequest);

        std::string host = extractHost(modifiedRequest.getUrl());
        int port = extractPort(modifiedRequest.getUrl());

        if (m_proxySettings)
        {
            host = m_proxySettings->host;
            port = m_proxySettings->port;
        }

        // ConnectionPool から接続を取得する際に、設定済みの WiFiSecureClient を生成するラムダ式を渡す
        auto connection = m_connectionPool->getConnection(
            host + ":" + std::to_string(port),
            [this]()
            {
                return m_wifiClient; // shared_ptr を直接返す
            });

        if (!connection->isConnected() && !connection->connect(host, port))
        {
            throwNetworkError("Failed to connect to " + host);
        }

        if (m_proxySettings)
        {
            // CONNECT メソッドを使用してプロキシトンネルを確立
            std::string connectRequest = "CONNECT " + extractHost(request.getUrl()) + ":" + std::to_string(extractPort(request.getUrl())) + " HTTP/1.1\r\n";
            connectRequest += "Host: " + extractHost(request.getUrl()) + ":" + std::to_string(extractPort(request.getUrl())) + "\r\n";

            if (!m_proxySettings->username.empty() && !m_proxySettings->password.empty())
            {
                std::string auth = m_proxySettings->username + ":" + m_proxySettings->password;
                connectRequest += "Proxy-Authorization: Basic " + base64Encode(auth) + "\r\n";
            }

            connectRequest += "\r\n";

            connection->write(reinterpret_cast<const uint8_t *>(connectRequest.c_str()), connectRequest.length());

            // プロキシからの応答を読み取り、200 OK を確認
            std::string response = connection->readLine();
            if (response.find("200") == std::string::npos)
            {
                throwNetworkError("Failed to establish proxy tunnel: " + response);
            }

            // 残りのヘッダーを読み飛ばす
            while (connection->readLine() != "\r\n")
            {
            }
        }

        std::string requestStr = buildRequestString(modifiedRequest);
        if (connection->write(reinterpret_cast<const uint8_t *>(requestStr.c_str()), requestStr.length()) != requestStr.length())
        {
            throwNetworkError("Failed to send request");
        }

        auto response = readStreamingResponse(connection.get(), modifiedRequest, chunkCallback);

        logResponse(*response);

        if (m_cookiesEnabled)
        {
            auto setCookieHeaders = response->getHeaders().equal_range("Set-Cookie");
            for (auto it = setCookieHeaders.first; it != setCookieHeaders.second; ++it)
            {
                m_cookieJar->setCookie(modifiedRequest.getUrl(), it->second);
            }
        }

        if (m_options.followRedirects && redirectCount < m_options.maxRedirects)
        {
            int statusCode = response->getStatusCode();
            if (statusCode >= 300 && statusCode < 400)
            {
                auto it = response->getHeaders().find("Location");
                if (it != response->getHeaders().end())
                {
                    Request redirectRequest = modifiedRequest;
                    redirectRequest.setUrl(it->second);
                    return sendStreamingWithRedirects(redirectRequest, chunkCallback, redirectCount + 1);
                }
            }
        }

        m_connectionPool->releaseConnection(host + ":" + std::to_string(port), connection);

        return response;
    }

    void HttpClient::applyAuthentication(Request &request)
    {
        switch (m_options.authType)
        {
        case AuthType::Basic:
        {
            std::string auth = m_options.username + ":" + m_options.password;
            std::string encodedAuth = base64Encode(auth);
            request.addHeader("Authorization", "Basic " + encodedAuth);
            break;
        }
        case AuthType::Bearer:
            request.addHeader("Authorization", "Bearer " + m_options.bearerToken);
            break;
        default:
            break;
        }
    }

    void HttpClient::logRequest(const Request &request)
    {
        if (m_logger)
        {
            std::ostringstream oss;
            oss << "Sending request: " << methodToString(request.getMethod()) << " " << request.getUrl();
            m_logger->log(oss.str());
        }
    }

    void HttpClient::logResponse(const Response &response)
    {
        if (m_logger)
        {
            std::ostringstream oss;
            oss << "Received response: Status " << response.getStatusCode();
            m_logger->log(oss.str());
        }
    }

    void HttpClient::cancel(const std::string &requestId)
    {
        // 実装は基盤となるネットワーク層に依存します
        // 現在は、単に切断します
        m_networkLayer->disconnect();
    }

    void HttpClient::setRateLimit(int requestsPerSecond)
    {
        // 単純なレート制限の実装
        // 実際のシナリオでは、トークンバケットアルゴリズムを使用することをお勧めします
        static std::chrono::steady_clock::time_point lastRequest;
        std::chrono::milliseconds interval(1000 / requestsPerSecond);

        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastRequest = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRequest);

        if (timeSinceLastRequest < interval)
        {
            std::this_thread::sleep_for(interval - timeSinceLastRequest);
        }

        lastRequest = std::chrono::steady_clock::now();
    }

    std::unique_ptr<Response> HttpClient::readResponse(INetworkLayer *connection, const Request &request)
    {
        auto response = std::make_unique<Response>();

        std::string statusLine = connection->readLine();
        parseStatusLine(statusLine, *response);

        // ヘッダーの読み取り
        std::string headerLine;
        while ((headerLine = connection->readLine()) != "\r\n")
        {
            parseHeader(headerLine, *response);
        }

        // レスポンスがチャンク化されているかチェック
        auto transferEncodingIt = response->getHeaders().find("Transfer-Encoding");
        if (transferEncodingIt != response->getHeaders().end() && transferEncodingIt->second == "chunked")
        {
            handleChunkedResponse(connection, *response);
        }
        else
        {
            // ボディの読み取り
            auto contentLengthIt = response->getHeaders().find("Content-Length");
            if (contentLengthIt != response->getHeaders().end())
            {
                size_t contentLength = std::stoul(contentLengthIt->second);
                std::vector<char> buffer(8192); // 8KBバッファ
                size_t totalRead = 0;

                while (totalRead < contentLength)
                {
                    size_t bytesToRead = std::min(buffer.size(), contentLength - totalRead);
                    size_t bytesRead = connection->read(reinterpret_cast<uint8_t *>(buffer.data()), bytesToRead);

                    if (bytesRead == 0)
                    {
                        break; // 接続が閉じられたかエラー
                    }

                    if (m_responseBodyCallback)
                    {
                        m_responseBodyCallback(buffer.data(), bytesRead);
                    }
                    else
                    {
                        response->appendToBody(std::string(buffer.data(), bytesRead));
                    }

                    totalRead += bytesRead;

                    if (m_progressCallback)
                    {
                        m_progressCallback(totalRead, contentLength);
                    }
                }
            }
            else
            {
                // Content-Lengthヘッダーがない場合
                std::vector<char> buffer(8192); // 8KBバッファ
                while (true)
                {
                    size_t bytesRead = connection->read(reinterpret_cast<uint8_t *>(buffer.data()), buffer.size());
                    if (bytesRead == 0)
                    {
                        break; // 接続が閉じられた
                    }

                    if (m_responseBodyCallback)
                    {
                        m_responseBodyCallback(buffer.data(), bytesRead);
                    }
                    else
                    {
                        response->appendToBody(std::string(buffer.data(), bytesRead));
                    }

                    if (m_progressCallback)
                    {
                        m_progressCallback(response->getBody().length(), 0); // 0 for unknown total size
                    }
                }
            }
        }

        return response;
    }

    std::unique_ptr<Response> HttpClient::readStreamingResponse(INetworkLayer *connection, const Request &request, ChunkCallback chunkCallback)
    {
        auto response = std::make_unique<Response>();

        std::string statusLine = connection->readLine();
        parseStatusLine(statusLine, *response);

        // ヘッダーの読み取り
        std::string headerLine;
        while ((headerLine = connection->readLine()) != "\r\n")
        {
            parseHeader(headerLine, *response);
        }

        // レスポンスがチャンク化されているかチェック
        auto transferEncodingIt = response->getHeaders().find("Transfer-Encoding");
        if (transferEncodingIt != response->getHeaders().end() && transferEncodingIt->second == "chunked")
        {
            handleChunkedResponse(connection, *response);
        }
        else
        {
            // ボディの読み取り
            std::vector<char> buffer(8192); // 8KBバッファ
            size_t totalRead = 0;

            while (true)
            {
                size_t bytesRead = connection->read(reinterpret_cast<uint8_t *>(buffer.data()), buffer.size());
                if (bytesRead == 0)
                {
                    break; // 接続が閉じられた
                }

                chunkCallback(buffer.data(), bytesRead);

                totalRead += bytesRead;

                if (m_progressCallback)
                {
                    m_progressCallback(totalRead, 0); // 0 for unknown total size
                }
            }
        }

        return response;
    }

    void HttpClient::handleChunkedResponse(INetworkLayer *connection, Response &response)
    {
        std::vector<char> buffer(8192); // 8KBバッファ
        size_t totalRead = 0;

        while (true)
        {
            std::string chunkSizeLine = connection->readLine();
            size_t chunkSize = std::stoul(chunkSizeLine, nullptr, 16);

            if (chunkSize == 0)
            {
                break; // 最後のチャンク
            }

            size_t chunkRead = 0;
            while (chunkRead < chunkSize)
            {
                size_t bytesToRead = std::min(buffer.size(), chunkSize - chunkRead);
                size_t bytesRead = connection->read(reinterpret_cast<uint8_t *>(buffer.data()), bytesToRead);

                if (bytesRead == 0)
                {
                    throw HttpError(HttpError::ErrorCode::NetworkError, "Connection closed unexpectedly");
                }

                if (m_responseBodyCallback)
                {
                    m_responseBodyCallback(buffer.data(), bytesRead);
                }
                else
                {
                    response.appendToBody(std::string(buffer.data(), bytesRead));
                }

                chunkRead += bytesRead;
                totalRead += bytesRead;

                if (m_progressCallback)
                {
                    m_progressCallback(totalRead, 0); // 0 for unknown total size
                }
            }

            // チャンクの終わりのCRLFを読み捨てる
            connection->readLine();
        }

        // トレーラーヘッダーがあれば読み取る
        std::string trailerLine;
        while ((trailerLine = connection->readLine()) != "\r\n")
        {
            parseHeader(trailerLine, response);
        }
    }

    void HttpClient::parseStatusLine(const std::string &statusLine, Response &response)
    {
        std::istringstream iss(statusLine);
        std::string httpVersion;
        int statusCode;
        std::string reasonPhrase;

        iss >> httpVersion >> statusCode;
        std::getline(iss, reasonPhrase);

        response.setStatusCode(statusCode);
    }

    void HttpClient::parseHeader(const std::string &headerLine, Response &response)
    {
        auto colonPos = headerLine.find(':');
        if (colonPos != std::string::npos)
        {
            std::string key = headerLine.substr(0, colonPos);
            std::string value = headerLine.substr(colonPos + 1);
            value.erase(0, value.find_first_not_of(" "));
            value.erase(value.find_last_not_of("\r\n") + 1);
            response.addHeader(key, value);
        }
    }

    std::string HttpClient::buildRequestString(const Request &request)
    {
        std::ostringstream oss;
        oss << methodToString(request.getMethod()) << " " << extractPath(request.getUrl()) << " HTTP/1.1\r\n";
        oss << "Host: " << extractHost(request.getUrl()) << "\r\n";

        const auto &multipartFormData = request.getMultipartFormData();
        if (!multipartFormData.empty())
        {
            std::string boundary = generateBoundary();
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
                auto cookies = m_cookieJar->getCookiesForUrl(request.getUrl());
                if (!cookies.empty())
                {
                    oss << "Cookie: " << joinStrings(cookies, "; ") << "\r\n";
                }
            }

            oss << "Content-Length: " << request.getBody().length() << "\r\n";
            oss << "\r\n";
            oss << request.getBody();
        }

        return oss.str();
    }

    std::string HttpClient::methodToString(Request::Method method)
    {
        switch (method)
        {
        case Request::Method::GET:
            return "GET";
        case Request::Method::POST:
            return "POST";
        case Request::Method::PUT:
            return "PUT";
        case Request::Method::DELETE:
            return "DELETE";
        case Request::Method::PATCH:
            return "PATCH";
        case Request::Method::HEAD:
            return "HEAD";
        case Request::Method::OPTIONS:
            return "OPTIONS";
        default:
            return "GET";
        }
    }

    std::string HttpClient::extractHost(const std::string &url)
    {
        std::string::size_type protocolEnd = url.find("://");
        if (protocolEnd == std::string::npos)
        {
            protocolEnd = 0;
        }
        else
        {
            protocolEnd += 3;
        }
        std::string::size_type pathStart = url.find('/', protocolEnd);
        if (pathStart == std::string::npos)
        {
            return url.substr(protocolEnd);
        }
        return url.substr(protocolEnd, pathStart - protocolEnd);
    }

    int HttpClient::extractPort(const std::string &url)
    {
        std::string host = extractHost(url);
        std::string::size_type colonPos = host.find(':');
        if (colonPos != std::string::npos)
        {
            return std::stoi(host.substr(colonPos + 1));
        }
        return url.substr(0, 5) == "https" ? 443 : 80;
    }

    std::string HttpClient::extractPath(const std::string &url)
    {
        std::string::size_type protocolEnd = url.find("://");
        if (protocolEnd == std::string::npos)
        {
            protocolEnd = 0;
        }
        else
        {
            protocolEnd += 3;
        }
        std::string::size_type pathStart = url.find('/', protocolEnd);
        if (pathStart == std::string::npos)
        {
            return "/";
        }
        return url.substr(pathStart);
    }

    std::string HttpClient::joinStrings(const std::vector<std::string> &strings, const std::string &delimiter)
    {
        std::ostringstream oss;
        for (size_t i = 0; i < strings.size(); ++i)
        {
            if (i > 0)
            {
                oss << delimiter;
            }
            oss << strings[i];
        }
        return oss.str();
    }

    std::string HttpClient::base64Encode(const std::string &input)
    {
        static const char base64_chars[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        std::string ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];
        unsigned int in_len = input.size();
        const unsigned char *bytes_to_encode = reinterpret_cast<const unsigned char *>(input.c_str());

        while (in_len--)
        {
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3)
            {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (i = 0; i < 4; i++)
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i)
        {
            for (j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (j = 0; j < i + 1; j++)
                ret += base64_chars[char_array_4[j]];

            while (i++ < 3)
                ret += '=';
        }

        return ret;
    }

    void HttpClient::setConnectionTimeout(std::chrono::milliseconds timeout)
    {
        m_connectionTimeout = timeout;
    }

    void HttpClient::setReadTimeout(std::chrono::milliseconds timeout)
    {
        m_readTimeout = timeout;
    }

    void HttpClient::setWriteTimeout(std::chrono::milliseconds timeout)
    {
        m_writeTimeout = timeout;
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

    void HttpClient::throwNetworkError(const std::string &message)
    {
        throw HttpError(HttpError::ErrorCode::NetworkError, "Network error: " + message);
    }

    void HttpClient::throwTimeoutError(const std::string &message)
    {
        throw HttpError(HttpError::ErrorCode::Timeout, "Timeout error: " + message);
    }

    void HttpClient::throwSSLError(const std::string &message)
    {
        throw HttpError(HttpError::ErrorCode::SSLError, "SSL error: " + message);
    }

    void HttpClient::throwInvalidResponseError(const std::string &message)
    {
        throw HttpError(HttpError::ErrorCode::InvalidResponse, "Invalid response: " + message);
    }

    void HttpClient::throwTooManyRedirectsError(const std::string &message)
    {
        throw HttpError(HttpError::ErrorCode::TooManyRedirects, "Too many redirects: " + message);
    }

    std::string HttpClient::generateBoundary()
    {
        static const char *chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        std::string result;
        result.reserve(16);
        for (int i = 0; i < 16; ++i)
        {
            result += chars[rand() % 62];
        }
        return result;
    }

    void HttpClient::setProxy(const std::string &host, int port, const std::string &username, const std::string &password)
    {
        m_proxySettings = ProxySettings{host, port, username, password};
    }

} // namespace http