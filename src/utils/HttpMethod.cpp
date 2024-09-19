#include "HttpMethod.h"
#include <algorithm>
#include <stdexcept>

namespace canaspad
{

    HttpMethod stringToHttpMethod(const std::string &methodStr)
    {
        // 文字列を大文字に変換
        std::string upperMethodStr = methodStr;
        std::transform(upperMethodStr.begin(), upperMethodStr.end(), upperMethodStr.begin(), ::toupper);

        if (upperMethodStr == "GET")
            return HttpMethod::GET;
        if (upperMethodStr == "POST")
            return HttpMethod::POST;
        if (upperMethodStr == "PUT")
            return HttpMethod::PUT;
        if (upperMethodStr == "DELETE")
            return HttpMethod::DELETE;
        if (upperMethodStr == "PATCH")
            return HttpMethod::PATCH;
        if (upperMethodStr == "HEAD")
            return HttpMethod::HEAD;
        if (upperMethodStr == "OPTIONS")
            return HttpMethod::OPTIONS;
        if (upperMethodStr == "CONNECT")
            return HttpMethod::CONNECT;
        if (upperMethodStr == "TRACE")
            return HttpMethod::TRACE;

        // 不明なメソッド
        throw std::invalid_argument("Invalid HTTP method: " + methodStr);
    }

    std::string httpMethodToString(HttpMethod method)
    {
        switch (method)
        {
        case HttpMethod::GET:
            return "GET";
        case HttpMethod::POST:
            return "POST";
        case HttpMethod::PUT:
            return "PUT";
        case HttpMethod::DELETE:
            return "DELETE";
        case HttpMethod::PATCH:
            return "PATCH";
        case HttpMethod::HEAD:
            return "HEAD";
        case HttpMethod::OPTIONS:
            return "OPTIONS";
        case HttpMethod::CONNECT:
            return "CONNECT";
        case HttpMethod::TRACE:
            return "TRACE";
        default:
            return ""; // 不明なメソッド
        }
    }

} // namespace canaspad