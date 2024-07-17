// ILogger.h
#pragma once
#include <string>

namespace http
{

    class ILogger
    {
    public:
        virtual ~ILogger() = default;
        virtual void log(const std::string &message) = 0;
    };

} // namespace http