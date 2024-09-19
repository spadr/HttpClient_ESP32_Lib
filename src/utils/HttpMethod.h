#pragma once

#include <string>

namespace canaspad
{

    enum class HttpMethod
    {
        GET,
        POST,
        PUT,
        DELETE,
        PATCH,
        HEAD,
        OPTIONS,
        CONNECT,
        TRACE,
    };

    // 文字列をHttpMethodに変換する関数
    HttpMethod stringToHttpMethod(const std::string &methodStr);

    // HttpMethodを文字列に変換する関数
    std::string httpMethodToString(HttpMethod method);

} // namespace canaspad