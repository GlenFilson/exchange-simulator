#pragma once
#include "message.hpp"
#include <vector>
#include <cstdint>

struct InboundMessage{//total 61 bytes
    int fd;//4 bytes
    MessageType type;//1 bytes
    size_t size;//8 bytes
    uint8_t data[48];//48 bytes
};