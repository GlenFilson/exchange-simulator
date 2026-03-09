#pragma once

#include "message.hpp"
#include <cstdint>

bool read_exact(int fd, uint8_t* buffer, size_t n);
void send_message(int fd, const Message& message);
bool read_message(int fd, Message& message);
