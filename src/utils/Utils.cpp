#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>

namespace canaspad
{

    std::string Utils::methodToString(Request::Method method)
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

    std::string Utils::extractHost(const std::string &url)
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

    int Utils::extractPort(const std::string &url)
    {
        std::string host = extractHost(url);
        std::string::size_type colonPos = host.find(':');
        if (colonPos != std::string::npos)
        {
            return std::stoi(host.substr(colonPos + 1));
        }
        return url.substr(0, 5) == "https" ? 443 : 80;
    }

    std::string Utils::extractPath(const std::string &url)
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

    std::string Utils::extractBaseUrl(const std::string &url)
    {
        // プロトコル部分の終わりを探す
        std::string::size_type protocolEnd = url.find("://");
        if (protocolEnd == std::string::npos)
        {
            protocolEnd = 0;
        }
        else
        {
            protocolEnd += 3;
        }

        // 最初の "/" の位置を探す。ただし、プロトコル部分の後の "//" はスキップする
        std::string::size_type pathStart = url.find('/', protocolEnd + 2);

        // "/" が見つからない場合、URL 全体がベース URL
        if (pathStart == std::string::npos)
        {
            return url;
        }

        return url.substr(0, pathStart);
    }

    std::string Utils::joinStrings(const std::vector<std::string> &strings, const std::string &delimiter)
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

    std::string Utils::base64Encode(const std::string &input)
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

    std::string Utils::generateBoundary()
    {
        static const char *chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        std::string result;
        result.reserve(16);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(0, 61);
        for (int i = 0; i < 16; ++i)
        {
            result += chars[distrib(gen)];
        }
        return result;
    }

    void Utils::parseStatusLine(const std::string &statusLine, HttpResult &result)
    {
        std::istringstream iss(statusLine);
        std::string httpVersion;
        iss >> httpVersion >> result.statusCode;
        std::getline(iss, result.statusMessage);
    }

    void Utils::parseHeader(const std::string &headerLine, HttpResult &result)
    {
        auto colonPos = headerLine.find(':');
        if (colonPos != std::string::npos)
        {
            std::string key = headerLine.substr(0, colonPos);
            std::string value = headerLine.substr(colonPos + 1);
            value.erase(0, value.find_first_not_of(" "));
            value.erase(value.find_last_not_of("\r\n") + 1);
            result.headers[key] = value;
        }
    }

    void Utils::parseCookie(const std::string &setCookieHeader, Cookie &cookie, const std::string &requestUrl)
    {
        std::istringstream iss(setCookieHeader);
        std::string token;

        std::getline(iss, token, '=');
        cookie.name = token;
        std::getline(iss, token, ';');
        cookie.value = token;

        while (std::getline(iss, token, ';'))
        {
            token.erase(0, token.find_first_not_of(" "));
            if (token.substr(0, 7) == "Domain=")
            {
                cookie.domain = token.substr(7);
            }
            else if (token.substr(0, 5) == "Path=")
            {
                cookie.path = token.substr(5);
            }
            else if (token == "Secure")
            {
                cookie.secure = true;
            }
            else if (token == "HttpOnly")
            {
                cookie.httpOnly = true;
            }
            else if (token.substr(0, 7) == "Expires=")
            {
                // Expires 属性の処理
                std::tm tm = {};
                std::istringstream ss(token.substr(8));
                ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S %Z");
                cookie.expires = std::mktime(&tm);
            }
        }

        // Domain 属性が指定されていない場合、リクエストURLのホスト部分を設定
        if (cookie.domain.empty())
        {
            cookie.domain = Utils::extractHost(requestUrl);
        }
    }

    std::string Utils::extractHeaderValue(const std::unordered_map<std::string, std::string> &headers, const std::string &key)
    {
        auto it = headers.find(key);
        if (it != headers.end())
        {
            return it->second;
        }
        return "";
    }

    std::vector<std::string> Utils::extractHeaders(const std::unordered_map<std::string, std::string> &headers, const std::string &key)
    {
        std::vector<std::string> result;
        auto range = headers.equal_range(key);
        for (auto it = range.first; it != range.second; ++it)
        {
            result.push_back(it->second);
        }
        return result;
    }

    size_t Utils::extractContentLength(const std::unordered_map<std::string, std::string> &headers)
    {
        auto it = headers.find("Content-Length");
        if (it != headers.end())
        {
            return std::stoul(it->second);
        }
        return 0;
    }

} // namespace canaspad