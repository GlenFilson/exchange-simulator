// common type aliases for easy migration
#pragma once
#include <cstdint>

using OrderId = uint64_t;
using Quantity = uint32_t;
using Timestamp = uint64_t;
using Price = uint64_t; // alias for now; change to fixed-point later if desired
