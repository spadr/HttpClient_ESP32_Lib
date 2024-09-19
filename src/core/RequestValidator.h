#pragma once

#include "../Result.h"
#include "../core/Request.h"

namespace canaspad
{

    class RequestValidator
    {
    public:
        static Result<void> validate(const Request &request, const ClientOptions &options);

    private:
        static Result<void> validateUrl(const std::string &url);
        static Result<void> validateMethod(HttpMethod method);
        static Result<void> validateClientOptions(const ClientOptions &options);
    };

} // namespace canaspad