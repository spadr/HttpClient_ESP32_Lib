#include "ResponseInjector.h"

#include <cstdint>

namespace canaspad
{

    void ResponseInjector::queueResponse(const std::vector<std::uint8_t> &response)
    {
        responses.push(response);
    }

    std::vector<std::uint8_t> ResponseInjector::getNextResponse()
    {
        if (responses.empty())
        {
            return {};
        }
        auto response = responses.front();
        responses.pop();
        return response;
    }

} // namespace canaspad