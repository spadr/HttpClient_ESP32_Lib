// Request.h
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace http
{

    class Request
    {
    public:
        enum class Method
        {
            GET,
            POST,
            PUT,
            DELETE,
            PATCH,
            HEAD,
            OPTIONS
        };

        Request();

        Request &setUrl(const std::string &url);
        Request &setMethod(Method method);
        Request &addHeader(const std::string &key, const std::string &value);
        Request &setBody(const std::string &body);
        Request &setMultipartFormData(const std::vector<std::pair<std::string, std::string>> &formData);

        const std::string &getUrl() const;
        Method getMethod() const;
        const std::unordered_map<std::string, std::string> &getHeaders() const;
        const std::string &getBody() const;
        const std::vector<std::pair<std::string, std::string>> &getMultipartFormData() const;

    private:
        std::string m_url;
        Method m_method;
        std::unordered_map<std::string, std::string> m_headers;
        std::string m_body;
        std::vector<std::pair<std::string, std::string>> m_multipartFormData;
    };

} // namespace http