#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "../core/Request.h"
#include "../cookie/Cookie.h"
#include "../core/HttpResult.h"

namespace canaspad
{

    class Utils
    {
    public:
        static std::string extractScheme(const std::string &url);
        static std::string extractHost(const std::string &url);
        static int extractPort(const std::string &url);
        static std::string extractPath(const std::string &url);
        static std::string extractBaseUrl(const std::string &url);
        static std::string joinStrings(const std::vector<std::string> &strings, const std::string &delimiter);
        static std::string base64Encode(const std::string &input);
        static std::string generateBoundary();
        static void parseStatusLine(const std::string &statusLine, HttpResult &result);
        static void parseHeader(const std::string &headerLine, HttpResult &result);
        static void parseCookie(const std::string &setCookieHeader, Cookie &cookie, const std::string &requestUrl);
        static std::string extractHeaderValue(const std::unordered_map<std::string, std::string> &headers, const std::string &key);
        static std::vector<std::string> extractHeaders(const std::unordered_map<std::string, std::string> &headers, const std::string &key);
        static size_t extractContentLength(const std::unordered_map<std::string, std::string> &headers);
    };

} // namespace canaspad