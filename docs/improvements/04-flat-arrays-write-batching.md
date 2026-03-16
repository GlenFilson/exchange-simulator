# Optimistic Sends, Flat Arrays & Write Batching

## The Problem
After implementing epoll, profiling revealed that infrastructure overhead — not business logic — dominated the CPU profile. Two functions alone consumed 40% of all CPU cycles.

**`perf report` excerpt:**
```text
 29.36%  exchange  libc.so.6   [.] epoll_ctl
  8.82%  exchange  exchange    [.] std::_Hashtable<unsigned long, std::pair<...
  1.72%  exchange  exchange    [.] std::_Hashtable<unsigned long, std::pair<...
```

## Investigation

### epoll_ctl: 29% of CPU
The server toggled `EPOLLOUT` on every message — once to register after buffering a response, once to deregister after sending. Each toggle is an `epoll_ctl` syscall. At ~130K+ messages/sec, that's ~260K+ unnecessary syscalls per second.

### Hash map lookups: ~11% of CPU
`std::unordered_map<int, ClientState>` was looked up 5+ times per message across `handle_read`, `handle_message`, `try_send`, and counterparty notifications. File descriptors are small sequential integers (typically 3–20) — perfect for direct array indexing instead of hashing.

### Inline sending: a failed first attempt
Replacing `EPOLLOUT` toggling with direct `send()` calls inside `handle_message` improved single-client throughput but caused a multi-client regression: 134,735 → 127,901 ops/sec. 

The reason: inline sending interleaves reads and writes (`recv-send-recv-send-recv-send`). The old `EPOLLOUT` approach accidentally batched them (`recv-recv-recv` then `send-send-send`), which is faster because grouped syscalls of the same type are more efficient.

### The Redis beforeSleep pattern
Research into how Redis handles this revealed a clean solution. Redis never sends responses during read processing. It buffers all responses, then flushes every pending write in one batch *before* calling `epoll_wait`. This gets the batching benefits of `EPOLLOUT` without any `epoll_ctl` syscalls.

## The Fix

### 1. Flat array for client state
Replaced the hash map with a `std::vector<ClientState>` pre-allocated to 1024 entries, indexed directly by file descriptor. Added an `active` flag to distinguish connected from empty slots.

```cpp
// before: hash, traverse bucket, follow pointers
auto it = clients_.find(fd);
if (it == clients_.end()) return;
ClientState& client = it->second;

// after: single array access
if (!clients_[fd].active) return;
ClientState& client = clients_[fd];
```

### 2. Optimistic try_send
`try_send(int fd)` attempts an optimistic, non-blocking `send()`. If the kernel accepts the entire buffer, it is cleared immediately - no `epoll_ctl` needed. The server only falls back to registering `EPOLLOUT` if the socket buffer is full (`EAGAIN` / partial send).

### 3. Deferred write flushing
`handle_message` no longer sends anything. It buffers responses and pushes the fd onto a `pending_writes_` list. Before each `epoll_wait`, `flush_pending_writes()` sends all buffered data in a tight loop.

```cpp
void ExchangeServer::run(){
    epoll_event events[64];
    while(true){
        flush_pending_writes();   // send all buffered responses
        int n = epoll_wait(epoll_fd_, events, 64, -1);
        for (int i = 0; i < n; i++){
            // read and process only — no sending here
        }
    }
}
```

All reads from one `epoll_wait` cycle are processed before any writes happen. More data accumulates in each client's buffer, so each `send()` pushes more data per syscall.

## Results

| Configuration | 1 Client | 5 Clients |
|---|---|---|
| epoll + EPOLLOUT toggle + hash map | 47,801 | 134,735 |
| try_send + flat array (inline sends) | 50,687 | 127,901 |
| try_send + flat array + write batching | **50,617** | **152,449** |

* `epoll_ctl` gone from the profile (was 29%)
* `std::_Hashtable` for `clients_` gone from the profile (was ~11%)
* Multi-client: 134,735 → 152,449 ops/sec (**+13%**)
* Single-client: 47,801 → 50,617 ops/sec (**+6%**)

## Next Steps
The profile is now dominated by actual business logic (`handle_message` 15.3%, `serialize_trade` 7.0%, `MatchingEngine::match` 3.7%). Remaining overhead comes from `order_to_client_fd_` (the last `unordered_map`, still at 12.4%) and `malloc`/`free` (~7.2%) from the `Message` payload copy and per-match `vector<Trade>` allocation.