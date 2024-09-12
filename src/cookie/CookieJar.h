#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "Cookie.h"

namespace canaspad
{

    class CookieJar
    {
    public:
        void setCookie(const std::string &url, const std::string &setCookieHeader);
        std::vector<std::string> getCookiesForUrl(const std::string &url) const;

    private:
        std::unordered_map<std::string, std::vector<Cookie>> m_cookies;
        void cleanupExpiredCookies();
    };

} // namespace canaspad