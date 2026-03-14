#pragma once

#include "message.hpp"
#include <cstdint>
#include <vector>

bool read_exact(int fd, uint8_t* buffer, size_t n);
bool read_message(int fd, Message& message);
