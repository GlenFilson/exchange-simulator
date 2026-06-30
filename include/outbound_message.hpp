#pragma once
#include <vector>
#include <cstdint>

struct OutboundMessage{//total: 60 bytes
    int fd;//4 bytes
    size_t size;//8 bytes
    uint8_t data[48];//48 bytes
};