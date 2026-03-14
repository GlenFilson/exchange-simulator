# TCP_NODELAY & Single Send Buffer

## The Problem
Throughput was severely degraded at ~12 orders/sec. Messages exhibited noticeable visual latency, with round trips taking roughly 82ms despite operating entirely on `localhost`.

## Investigation
Wall-clock profiling showed the client spending 99.85% of its time blocked in `recv`. A review of the networking layer revealed that the server was making 3 separate `send()` syscalls per message (1 byte for type, 4 bytes for length, ~22 bytes for payload). This fragmented payload triggered the OS-level Nagle's Algorithm, which artificially delayed small packet transmission in an attempt to batch network traffic.

## The Fix
1. Disabled Nagle's algorithm by setting the `TCP_NODELAY` socket option to force immediate transmission.
2. Refactored the serialization layer to construct the entire wire format (type, length, and payload) into a single contiguous memory block (`std::vector<uint8_t>`).
3. Reduced network output to a single `send()` syscall per message, drastically reducing syscall overhead.

**Before:**
```cpp
// 3 separate syscalls trigger Nagle's algorithm
send(fd, &type, sizeof(MessageType), 0);
send(fd, &length, sizeof(uint32_t), 0);
send(fd, payload.data(), payload.size(), 0);
```

**After:**
```cpp
// disable Nagle's Algorithm
int flag = 1;
setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

// serialize to contiguous buffer and execute a single syscall
serializer.serialize_message(type, payload, write_buffer);
send(fd, write_buffer.data(), write_buffer.size(), 0);
```

## Results
* **Throughput:** 12 ops/sec $\rightarrow$ 46,153 ops/sec (**~3,800x increase**)

## Next Steps
With the artificial network delay removed, the CPU is now saturated enough to meaningfully profile the application code. Profiling points to memory allocations as the primary bottleneck on the hot path.