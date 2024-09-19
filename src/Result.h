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
        ProxyAuthenticationRequired,
        MissingHeader,
        InvalidHeader,
        DuplicateHeader,
        InvalidBody,
        InvalidOption,
        InvalidProxyURL
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
        Result(const ErrorInfo &error) : m_data(error) {}

        // コピーコンストラクタ
        Result(const Result &other) : m_data(other.m_data) {}
        // ムーブ代入演算子
        Result &operator=(Result &&other) noexcept
        {
            if (this != &other)
            {
                m_data = std::move(other.m_data);
            }
            return *this;
        }

        bool isSuccess() const { return std::holds_alternative<T>(m_data); }
        bool isError() const { return std::holds_alternative<ErrorInfo>(m_data); }

        // value() は T 型の Result でのみ使用されるため、特殊化しない
        const T &value() const & { return std::get<T>(m_data); }
        T &&value() && { return std::move(std::get<T>(m_data)); }

        const ErrorInfo &error() const { return std::get<ErrorInfo>(m_data); }
    };

    // void 型の特殊化
    template <>
    class Result<void>
    {
        std::variant<std::monostate, ErrorInfo> m_data;

    public:
        Result() : m_data(std::monostate{}) {} // 成功時のコンストラクタ
        Result(ErrorInfo &&error) : m_data(std::move(error)) {}
        Result(const ErrorInfo &error) : m_data(error) {}

        // コピーコンストラクタ
        Result(const Result &other) : m_data(other.m_data) {}
        // ムーブ代入演算子
        Result &operator=(Result &&other) noexcept
        {
            if (this != &other)
            {
                m_data = std::move(other.m_data);
            }
            return *this;
        }

        bool isSuccess() const { return std::holds_alternative<std::monostate>(m_data); }
        bool isError() const { return std::holds_alternative<ErrorInfo>(m_data); }

        const ErrorInfo &error() const { return std::get<ErrorInfo>(m_data); }
    };

} // namespace canaspad