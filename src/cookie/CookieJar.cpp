#include "CookieJar.h"
#include <algorithm>
#include <sstream>
#include <ctime>
#include <iomanip>
#include "../utils/Utils.h"

namespace canaspad
{

    void CookieJar::setCookie(const std::string &url, const std::string &setCookieHeader)
    {
        Cookie cookie;
        Utils::parseCookie(setCookieHeader, cookie);
        m_cookies[Utils::extractBaseUrl(url)].push_back(cookie);
    }

    std::vector<std::string> CookieJar::getCookiesForUrl(const std::string &url) const
    {
        std::vector<std::string> result;
        time_t now = time(nullptr);

        for (const auto &pair : m_cookies)
        {
            if (url.find(pair.first) != std::string::npos)
            {
                for (const auto &cookie : pair.second)
                {
                    // 有効期限が設定されていて、現在時刻より前の場合はスキップ
                    if (cookie.expires != 0 && cookie.expires < now)
                    {
                        continue;
                    }
                    result.push_back(cookie.name + "=" + cookie.value);
                }
            }
        }
        return result;
    }
} // namespace canaspad