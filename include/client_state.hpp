#pragma once
#include <vector>
#include <cstdint>
#include "message.hpp"

enum class ReadPhase{HEADER, PAYLOAD};

struct ClientState{
    ReadPhase read_phase = ReadPhase::HEADER;
    MessageType message_type;
    uint8_t header_buffer[5];//5 bytes 1 byte messag type + 4 byte message length
    std::vector<uint8_t> read_buffer;
    std::vector<uint8_t> write_buffer;
    uint32_t payload_length;
    size_t bytes_read = 0;
    size_t bytes_sent = 0;
    size_t send_size = 0;
};