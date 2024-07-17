// CookieJar.cpp
#include "CookieJar.h"
#include <algorithm>
#include <sstream>

namespace http
{

    void CookieJar::setCookie(const std::string &url, const std::string &setCookieHeader)
    {
        Cookie cookie;
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
        }

        m_cookies[url].push_back(cookie);
    }

    std::vector<std::string> CookieJar::getCookiesForUrl(const std::string &url) const
    {
        std::vector<std::string> result;
        for (const auto &pair : m_cookies)
        {
            if (url.find(pair.first) != std::string::npos)
            {
                for (const auto &cookie : pair.second)
                {
                    result.push_back(cookie.name + "=" + cookie.value);
                }
            }
        }
        return result;
    }

} // namespace http