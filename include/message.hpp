    #pragma once

#include <vector>
#include <cstdint>

//MessageType is actually a single byte
enum class MessageType : uint8_t {
    NEW_ORDER = 1,
    CANCEL_ORDER = 2,
    ORDER_ACK = 3,
    REJECT = 4,
    TRADE = 5,
    CANCEL_ACK = 6
};

struct Message{
    MessageType type;
    std::vector<uint8_t> payload; //uint8_t, raw bytes
};
