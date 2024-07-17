// CookieJar.h
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace http
{

    class Cookie
    {
    public:
        std::string name;
        std::string value;
        std::string domain;
        std::string path;
        bool secure;
        bool httpOnly;
        time_t expires;
    };

    class CookieJar
    {
    public:
        void setCookie(const std::string &url, const std::string &setCookieHeader);
        std::vector<std::string> getCookiesForUrl(const std::string &url) const;

    private:
        std::unordered_map<std::string, std::vector<Cookie>> m_cookies;
    };

} // namespace http