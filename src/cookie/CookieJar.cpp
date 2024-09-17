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
        Utils::parseCookie(setCookieHeader, cookie, url); // リクエストURLを渡す

        // クッキーをドメイン単位で保存
        m_cookies[cookie.domain].push_back(cookie);

        // setCookie 内で有効期限切れのクッキーを削除 (例)
        cleanupExpiredCookies();
    }

    std::vector<std::string> CookieJar::getCookiesForUrl(const std::string &url) const
    {
        std::vector<std::string> result;
        time_t now = time(nullptr);
        std::string domain = Utils::extractHost(url); // URLからドメインを取得

        // ドメインに一致するクッキーを取得
        auto it = m_cookies.find(domain);
        if (it != m_cookies.end())
        {
            for (const auto &cookie : it->second)
            {
                if (cookie.expires == 0 || cookie.expires >= now)
                {
                    result.push_back(cookie.name + "=" + cookie.value);
                }
            }
        }

        return result;
    }

    // 有効期限切れのクッキーを削除する
    void CookieJar::cleanupExpiredCookies()
    {
        time_t now = time(nullptr);
        for (auto it = m_cookies.begin(); it != m_cookies.end();)
        {
            bool allExpired = true;
            for (auto cookieIt = it->second.begin(); cookieIt != it->second.end();)
            {
                if (cookieIt->expires != 0 && cookieIt->expires < now)
                {
                    // 有効期限切れのクッキーを削除
                    cookieIt = it->second.erase(cookieIt);
                }
                else
                {
                    allExpired = false;
                    ++cookieIt;
                }
            }

            // 全てのクッキーが有効期限切れの場合、エントリ自体を削除
            if (allExpired)
            {
                it = m_cookies.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

} // namespace canaspad