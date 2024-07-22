#pragma once

#include <cstdint>
#include <queue>
#include <vector>

namespace canaspad
{

    class ResponseInjector
    {
    public:
        void queueResponse(const std::vector<std::uint8_t> &response);
        std::vector<std::uint8_t> getNextResponse();

    private:
        std::queue<std::vector<std::uint8_t>> responses;
    };

} // namespace canaspad