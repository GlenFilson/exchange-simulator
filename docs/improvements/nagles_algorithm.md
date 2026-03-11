# TCP_NODELAY + Single Send Buffer

## Evidence

12 orders/sec. ~82ms per round trip. Wall-clock profiling showed 99.85% of time blocked in `recv`.

`send_message` was doing 3 separate `send()` calls per message (1 byte, 4 bytes, ~22 bytes).

## Analysis

Nagle's algorithm batches small TCP writes, adding ~40ms delay. Two directions = ~80ms round trip, matching the 82ms observed. 3 `send()` calls per message also meant 3 syscalls worth of kernel overhead.

## Fix

- `TCP_NODELAY` on all sockets
- Combined 3 sends into one buffer, one `send()` call

## Results

12 → 46,153 orders/sec (**3,846x**). Profile shifted from `recv` wait to `std::vector` allocations.
