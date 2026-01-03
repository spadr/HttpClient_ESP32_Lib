#include "RequestValidator.h"
#include <algorithm>
#include <cctype>
#include <set>
#include <cstring>

namespace canaspad
{

    Result<void> RequestValidator::validate(const Request &request,
                                            const ClientOptions &options)
    {
        auto result = validateUrl(request.getUrl());
        if (result.isError())
        {
            return result;
        }

        result = validateMethod(request.getMethod());
        if (result.isError())
        {
            return result;
        }

        result = validateHeaders(request);
        if (result.isError())
        {
            return result;
        }

        result = validateBody(request);
        if (result.isError())
        {
            return result;
        }

        result = validateClientOptions(options);
        if (result.isError())
        {
            return result;
        }

        return Result<void>();
    }

    Result<void> RequestValidator::validateUrl(const std::string &url)
    {
        // URL が空文字列かどうか
        if (url.empty())
        {
            return Result<void>(ErrorInfo(ErrorCode::InvalidURL, "URL is empty."));
        }

        // スキーマのチェック
        auto scheme = Utils::extractScheme(url);
        if (scheme.empty())
        {
            return Result<void>(
                ErrorInfo(ErrorCode::InvalidURL, "URL does not contain a scheme."));
        }
        if (scheme != "http" && scheme != "https")
        {
            return Result<void>(ErrorInfo(ErrorCode::UnsupportedProtocol,
                                          "URL scheme must be 'http' or 'https'."));
        }

        // ホスト名とポート番号のチェック
        auto host = Utils::extractHost(url);
        if (host.empty())
        {
            return Result<void>(ErrorInfo(ErrorCode::InvalidURL, "URL does not contain a host."));
        }
        int port = Utils::extractPort(url);
        if (port < 0 || port > 65535)
        {
            return Result<void>(ErrorInfo(ErrorCode::InvalidURL, "Invalid port number."));
        }

        // URL エンコードのチェック (簡易的なチェック)
        for (char c : url)
        {
            if (!std::isalnum(c) && !std::strchr(".-_~:/?#[]@!$&'()*+,;=", c))
            {
                return Result<void>(ErrorInfo(
                    ErrorCode::InvalidURL, "URL contains invalid or unencoded characters."));
            }
        }

        return Result<void>();
    }

    Result<void> RequestValidator::validateMethod(HttpMethod method)
    {
        // 現状ではすべての HttpMethod を有効とみなす
        return Result<void>();
    }

    Result<void> RequestValidator::validateClientOptions(
        const ClientOptions &options)
    {
        // プロキシ設定のチェック
        if (!options.proxyUrl.empty())
        {
            // スキーマのチェック
            auto scheme = Utils::extractScheme(options.proxyUrl);
            if (scheme.empty())
            {
                return Result<void>(
                    ErrorInfo(ErrorCode::InvalidProxyURL, "Proxy URL does not contain a scheme."));
            }
            if (scheme != "http" && scheme != "https")
            {
                return Result<void>(
                    ErrorInfo(ErrorCode::InvalidProxyURL, "Proxy URL must use the http/https scheme."));
            }
            // ホスト名のチェック
            auto host = Utils::extractHost(options.proxyUrl);
            if (host.empty())
            {
                return Result<void>(ErrorInfo(ErrorCode::InvalidProxyURL, "Proxy URL does not contain a host."));
            }
            // ポート番号のチェック
            int port = Utils::extractPort(options.proxyUrl);
            if (port < 0 || port > 65535)
            {
                return Result<void>(ErrorInfo(ErrorCode::InvalidProxyURL,
                                              "Proxy URL contains an invalid port number."));
            }
        }

        // リダイレクト設定のチェック
        if (options.maxRedirects < 0)
        {
            return Result<void>(ErrorInfo(ErrorCode::InvalidOption, 
                                          "Max redirects cannot be negative."));
        }

        // リトライ設定のチェック
        if (options.maxRetries < 0)
        {
            return Result<void>(ErrorInfo(ErrorCode::InvalidOption, 
                                          "Max retries cannot be negative."));
        }

        return Result<void>();
    }

    Result<void> RequestValidator::validateHeaders(const Request &request)
    {
        const auto& headers = request.getHeaders();
        
        for (const auto& header : headers)
        {
            // ヘッダー名が空の場合
            if (header.first.empty())
            {
                return Result<void>(ErrorInfo(ErrorCode::InvalidHeader, 
                                              "Header name cannot be empty."));
            }
        }
        
        return Result<void>();
    }

    Result<void> RequestValidator::validateBody(const Request &request)
    {
        // 現在の実装では、GETリクエストでもボディを許可する
        // 必要に応じて制約を追加可能
        return Result<void>();
    }

} // namespace canaspad