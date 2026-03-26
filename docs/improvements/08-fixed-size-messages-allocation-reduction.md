# Fixed-size Messages & Allocation Reduction

## The Problem
After achieving 750K ops/sec with client pipelining and read batching, profiling with `perf record -e cpu-clock` (which captures wall-clock time including kernel) showed the processing thread spending over 30% of CPU time in `malloc`, `free`, `operator new`, and `operator delete`. Every message flowing through the system created and destroyed heap-allocated `std::vector<uint8_t>` payloads. Every order resting on the book allocated a `std::list` node. Every match freed one.

**`perf report` (cpu-clock, 5 pipelined clients, before changes):**
```text
  28.59%  OrderProcessor::run()          (spin loop, idle)
  14.62%  __libc_recv                    (I/O syscall)
  ~22%    malloc/free/new/delete         (heap allocations)
   8.65%  epoll_wait                     (I/O syscall)
   ~7%    OrderProcessor::process + match + serialize (actual work)
```

The processing thread was doing 7% real work and 22% allocation overhead. Over 3x more time allocating memory than matching orders.

## Investigation

### Tracing every allocation for a single order

An order flowing through the system hit the heap at every stage:

1. `handle_read`: copies `client.read_buffer` (vector) into `InboundMessage.payload` (vector). Allocates.
2. `try_push` on ring buffer: copies the `InboundMessage`, copying the vector payload. Allocates again.
3. `OrderProcessor::process`: constructs `Message` with vector payload. Allocates.
4. `process`: creates `std::vector<uint8_t> buffer` for serialization output. Allocates.
5. `process`: creates `std::vector<uint8_t> cp_buffer` per trade for counterparty. Allocates per trade.
6. Every `OutboundMessage` with a vector payload pushed to the ring buffer. Allocates on copy.
7. `drain_outbound_queue`: inserts payload into `client.write_buffer`. May reallocate.

At least 4-6 heap allocations per order, millions of times per second.

### Matching engine isolation benchmark

Benchmarking the matching engine alone (no networking, no serialization) showed it could process **9.4 million orders/sec**. The server was achieving 750K through TCP. The gap was entirely overhead: allocations, syscalls, and serialization.

## The Fixes

### 1. Fixed-size messages (zero-allocation queue transport)

Replaced `std::vector<uint8_t>` payloads in `InboundMessage` and `OutboundMessage` with fixed-size byte arrays:

```cpp
struct InboundMessage {
    int fd;
    MessageType type;
    size_t size;
    uint8_t data[48];
};

struct OutboundMessage {
    int fd;
    size_t size;
    uint8_t data[48];
};
```

48 bytes fits any message (largest is a serialized trade at 41 bytes). The entire struct fits in a cache line. Pushing to the ring buffer is a single `memcpy` of flat memory. No heap, no allocator, no pointers.

### 2. Pre-allocated member buffers on OrderProcessor

Moved serialization buffers and the trades vector from local variables to member variables on `OrderProcessor`:

```cpp
std::vector<uint8_t> taker_buffer_;
std::vector<uint8_t> maker_buffer_;
Message deserialization_message_;
std::vector<Trade> trades_;
```

Each is allocated once on first use, then `clear()` reuses the existing capacity on every subsequent call. Changed `MatchingEngine::match` to take `std::vector<Trade>&` by reference instead of returning a new vector.

### 3. One outbound push per trade (Option B)

Instead of accumulating all trades + ACK into one large outbound message, each trade notification and the ACK are pushed as separate fixed-size messages. Simpler code, uniform message sizes, no variable-length accumulation.

### 4. Batched recv

Replaced the two-recv-per-message approach (one for 5-byte header, one for payload) with a large buffer recv. One `recv` call reads up to 4096 bytes, which can contain dozens of complete messages. A parse loop extracts all complete messages from the buffer without additional syscalls.

```cpp
struct ClientState {
    static constexpr size_t RECV_BUFFER_SIZE = 4096;
    uint8_t recv_buffer[RECV_BUFFER_SIZE];
    size_t recv_bytes = 0;
    size_t parse_offset = 0;
};
```

This reduced recv from 14.6% to 9.7% of wall-clock time (verified via cpu-clock profiling).

### 5. Integer prices

Changed prices from `double` to `int64_t`. With double prices from a normal distribution, virtually every order created a unique price level (a new map entry, a new list). With integer prices, orders cluster onto ~30 price levels. Fewer map insertions and deletions, fewer list node allocations.

This also fixes a correctness issue: comparing doubles for equality is unreliable due to floating point representation. Real exchanges use integer prices for this reason.

## Results

| Configuration | 1 Client | 5 Clients |
|---|---|---|
| Pipelined client + read batching (before) | 153,424 | 749,967 |
| + fixed-size messages + batched recv (after) | **154,482** | **755,003** |

Throughput was unchanged because the processing thread already had headroom. The optimizations made it faster, but it was already waiting for work most of the time.

**What changed under the hood:**

| Metric (5 clients) | Before | After |
|---|---|---|
| Server instructions | 128B | 131B |
| malloc/free (cpu-clock) | ~30% | ~22% |
| recv (cpu-clock) | 14.6% | 9.7% |
| Processing thread idle | 28.6% | 35.7% |

The processing thread became more efficient but the I/O thread is the ceiling. Both threads now have idle time, and the remaining cost is dominated by kernel syscalls (recv + epoll_wait = ~25% of wall-clock time).

## Bottleneck Analysis

At 755K ops/sec, the server's wall-clock time breaks down as:

```
Processing thread spin (idle):  ~36%
recv syscall:                   ~13%
epoll_wait:                     ~12%
malloc/free (structural):       ~22%
Actual processing work:          ~7%
Everything else:                ~10%
```

The I/O thread spends ~25% of wall-clock time in kernel syscalls. At 750K+ messages/sec with responses, that's roughly 2M syscall transitions per second, each costing 1-2us. This is a hard floor for userspace TCP on localhost.

The remaining allocations (~22%) come from order book data structures: `std::list<Order>` allocates a heap node for every order placed on the book and frees one for every match. `std::unordered_map` does the same for the order location map. These are structural to the data structures, not the message passing path.

## What would move the needle

Further gains from this point would require fundamentally different approaches:

- **Multiple I/O threads** to parallelize syscall overhead
- **Kernel bypass** (io_uring or DPDK) to eliminate syscall transitions entirely
- **Object pools** for order book nodes to eliminate structural allocations
- **Moving off localhost** where real network latency makes the pipelining gains more pronounced

## Next Steps

The TCP exchange architecture has been optimized to the point where the kernel networking stack is the dominant cost. The next feature will be multicast UDP market data, which is a completely different networking model and what real exchanges use for price distribution.