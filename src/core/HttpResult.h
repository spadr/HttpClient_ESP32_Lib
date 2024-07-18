#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "../cookie/Cookie.h"

namespace canaspad
{

    struct HttpResult
    {
        int statusCode;
        std::string statusMessage;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
        std::vector<Cookie> cookies;

        HttpResult(int code = 0) : statusCode(code) {}
    };

} // namespace canaspad