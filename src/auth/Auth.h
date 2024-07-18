#pragma once

#include <string>
#include "../core/Request.h"
#include "../core/CommonTypes.h"

namespace canaspad
{

    class Auth
    {
    public:
        Auth(const ClientOptions &options);

        void applyAuthentication(Request &request);

    private:
        AuthType m_authType;
        std::string m_username;
        std::string m_password;
        std::string m_bearerToken;
    };

} // namespace canaspad