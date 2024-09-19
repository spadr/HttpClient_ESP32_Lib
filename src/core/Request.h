#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "../core/CommonTypes.h"
#include "../utils/Utils.h"
#include "../utils/HttpMethod.h"

namespace canaspad
{

    class Request
    {
    public:
        canaspad::HttpMethod m_method;

        Request();

        Request &setUrl(const std::string &url);
        Request &setMethod(canaspad::HttpMethod method);
        Request &addHeader(const std::string &key, const std::string &value);
        Request &setBody(const std::string &body);
        Request &setMultipartFormData(const std::vector<std::pair<std::string, std::string>> &formData);

        const std::string &getUrl() const;
        canaspad::HttpMethod getMethod() const;
        const std::unordered_map<std::string, std::string> &getHeaders() const;
        const std::string &getBody() const;
        const std::vector<std::pair<std::string, std::string>> &getMultipartFormData() const;

    private:
        std::string m_url;
        std::unordered_map<std::string, std::string> m_headers;
        std::string m_body;
        std::vector<std::pair<std::string, std::string>> m_multipartFormData;
    };

} // namespace canaspad