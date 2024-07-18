#pragma once
#include <string>

namespace canaspad
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

} // namespace canaspad