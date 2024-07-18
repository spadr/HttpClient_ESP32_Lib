#pragma once

#include <variant>
#include <string>

namespace canaspad
{

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
        RequestCancelled,
        TimeNotSet,
        UnsupportedOperation,
    };

    struct ErrorInfo
    {
        ErrorCode code;
        std::string message;

        ErrorInfo() : code(ErrorCode::None), message("") {}
        ErrorInfo(ErrorCode c, std::string msg) : code(c), message(std::move(msg)) {}
    };

    template <typename T>
    class Result
    {
        std::variant<T, ErrorInfo> m_data;

    public:
        Result(T &&value) : m_data(std::move(value)) {}
        Result(const T &value) : m_data(value) {}
        Result(ErrorInfo &&error) : m_data(std::move(error)) {}

        bool isSuccess() const { return std::holds_alternative<T>(m_data); }
        bool isError() const { return std::holds_alternative<ErrorInfo>(m_data); }

        const T &value() const & { return std::get<T>(m_data); }
        T &&value() && { return std::move(std::get<T>(m_data)); }

        const ErrorInfo &error() const { return std::get<ErrorInfo>(m_data); }

        template <typename F>
        auto and_then(F &&f) const & -> Result<std::invoke_result_t<F, const T &>>
        {
            if (isSuccess())
            {
                return f(value());
            }
            return Result<std::invoke_result_t<F, const T &>>(error());
        }

        template <typename F>
        auto and_then(F &&f) && -> Result<std::invoke_result_t<F, T &&>>
        {
            if (isSuccess())
            {
                return f(std::move(*this).value());
            }
            return Result<std::invoke_result_t<F, T &&>>(std::move(*this).error());
        }

        template <typename F>
        auto map(F &&f) const & -> Result<std::invoke_result_t<F, const T &>>
        {
            if (isSuccess())
            {
                return Result<std::invoke_result_t<F, const T &>>(f(value()));
            }
            return Result<std::invoke_result_t<F, const T &>>(error());
        }

        template <typename F>
        auto map(F &&f) && -> Result<std::invoke_result_t<F, T &&>>
        {
            if (isSuccess())
            {
                return Result<std::invoke_result_t<F, T &&>>(f(std::move(*this).value()));
            }
            return Result<std::invoke_result_t<F, T &&>>(std::move(*this).error());
        }

        T value_or(T &&default_value) const &
        {
            if (isSuccess())
            {
                return value();
            }
            return std::forward<T>(default_value);
        }

        T value_or(T &&default_value) &&
        {
            if (isSuccess())
            {
                return std::move(*this).value();
            }
            return std::forward<T>(default_value);
        }
    };

} // namespace canaspad