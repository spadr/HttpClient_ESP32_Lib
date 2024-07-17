// HttpError.h
#pragma once
#include <stdexcept>
#include <string>

namespace http
{

    class HttpError : public std::runtime_error
    {
    public:
        enum class ErrorCode
        {
            None,
            NetworkError,
            Timeout,
            SSLError,
            InvalidResponse,
            TooManyRedirects,
            UnsupportedProtocol,
            InvalidURL,
            RequestCancelled
        };

        HttpError(ErrorCode code, const std::string &message)
            : std::runtime_error(message), m_errorCode(code) {}

        ErrorCode getErrorCode() const { return m_errorCode; }

    private:
        ErrorCode m_errorCode;
    };

} // namespace http