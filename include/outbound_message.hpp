#pragma once
#include <vector>
#include <cstdint>

struct OutboundMessage{
    int fd;
    std::vector<uint8_t> payload;
};