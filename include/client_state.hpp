#pragma once
#include <vector>
#include <cstdint>
#include "message.hpp"

// enum class ReadPhase{HEADER, PAYLOAD};

struct ClientState{
    //changing to storing ClientState in vector, need to mark each slot in the vector to know if its active or not
    //previously with map, every ClientState in the map was connected, we dont want to erase from middle of vector so we will keep states inside and simply mark them active or not
    bool active = false;

    static constexpr size_t RECV_BUFFER_SIZE = 4096;//one page
    uint8_t recv_buffer[RECV_BUFFER_SIZE];
    size_t recv_bytes = 0;//bytes in the buffer, how much data we received from recv
    size_t parse_offset = 0;//how much into the buffer we have read/processed

    std::vector<uint8_t> write_buffer;
    size_t sent_bytes = 0;

};