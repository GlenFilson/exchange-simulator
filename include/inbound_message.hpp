#pragma once
#include "message.hpp"
#include <vector>
#include <cstdint>

struct InboundMessage{
    int fd;
    MessageType type;
    std::vector<uint8_t> payload;
};