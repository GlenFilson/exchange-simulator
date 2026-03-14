# Pre-allocated Buffers & Zero-Allocation Hot Path

## The Problem
Processing 1 million orders resulted in massive CPU instruction counts. The server suffered from poor instruction cache locality and high overhead from continuous heap allocations.

## Investigation
Running `perf record` and inspecting the call graph revealed that dynamic memory allocation was dominating the application's runtime. Standard library allocator functions (`malloc`, `free`, and vector mutations) occupied the top execution hotspots. 

**`perf report` excerpt:**
```text
  4.14%  exchange  libstdc++.so.6.0.33   [.] std::vector<uint8_t...>::_M_default_append
  3.20%  exchange  libc.so.6             [.] malloc
  2.85%  exchange  libc.so.6             [.] _int_free
  1.56%  exchange  libc.so.6             [.] cfree@GLIBC_2.2.5
```
The `while(true)` loops for reading and writing were dynamically instantiating brand new `std::vector` and `Message` objects on strictly every iteration.

## The Fix
Achieved a zero-allocation hot path by moving the buffer and message declarations outside the I/O event loops. Calling `vector::clear()` and `vector::resize()` on pre-allocated buffers reuses the existing heap capacity, stripping standard library allocator latency out of the execution loop.

**Before:**
```cpp
while(true) {
    // allocates new heap memory on every single iteration
    std::vector<uint8_t> read_buffer; 
    Message message;
    
    recv(fd, read_buffer.data(), length, 0);
    // parse and handle message
}
```

**After:**
```cpp
// moved outside the event loops
std::vector<uint8_t> read_buffer; 
Message message; 

while(true) {
    // O(1) size reset, retains capacity, zero heap allocation
    read_buffer.resize(length); 
    
    recv(fd, read_buffer.data(), length, 0);
    // parse and handle message
}
```

## Results
* **Instructions executed (Server):** 10.3 Billion $\rightarrow$ 1.7 Billion (**83% reduction**)
* **Throughput:** 46,728 ops/sec $\rightarrow$ 50,643 ops/sec (+8.4%)

## Next Steps
While single-thread instruction efficiency vastly improved CPU utilization, the server is architecturally limited by its synchronous I/O loop. If a second client connects, the server completely hangs while blocking on the first client.