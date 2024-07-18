#include "Auth.h"
#include "../utils/Utils.h"
#include "../HttpClient.h"

namespace canaspad
{

    Auth::Auth(const ClientOptions &options) : m_authType(options.authType),
                                               m_username(options.username),
                                               m_password(options.password),
                                               m_bearerToken(options.bearerToken)
    {
    }

    void Auth::applyAuthentication(Request &request)
    {
        switch (m_authType)
        {
        case AuthType::Basic:
        {
            std::string auth = m_username + ":" + m_password;
            std::string encodedAuth = Utils::base64Encode(auth);
            request.addHeader("Authorization", "Basic " + encodedAuth);
            break;
        }
        case AuthType::Bearer:
            request.addHeader("Authorization", "Bearer " + m_bearerToken);
            break;
        default:
            break;
        }
    }

} // namespace canaspad