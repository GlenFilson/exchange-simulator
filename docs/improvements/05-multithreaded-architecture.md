# Multithreaded Architecture: I/O + Processing Split

## The Problem
The single-threaded server was hitting 98% CPU on one core. While matching an order, no reads or writes could happen. While sending responses, no new orders could be processed. Everything ran on one thread and there was no headroom left.

## Investigation
Real exchanges separate networking from order processing. One thread handles reads and writes, another runs the matching engine. This way both can happen at the same time: the I/O thread reads the next message while the processing thread matches the previous one.

This meant splitting `ExchangeServer` into two components:

```
ExchangeServer (I/O thread)         OrderProcessor (processing thread)
├── epoll event loop                 ├── OrderBook
├── clients_ vector                  ├── MatchingEngine
├── accept / read / write            ├── Serializer
├── inbound queue (push)             ├── order_to_client_fd_
└── outbound queue (pop)             ├── inbound queue (pop)
                                     └── outbound queue (push)
```

The two threads communicate via two `ThreadSafeQueue`s, a `std::queue` protected by a `std::mutex`. The I/O thread pushes raw messages onto the inbound queue and pops serialized responses from the outbound queue. The processing thread does the reverse.

## The Fix

### 1. ThreadSafeQueue
Built a generic thread-safe queue wrapping `std::queue` with `std::mutex`. `push()` locks, moves the item in, unlocks. `try_pop()` locks, checks if empty, moves the front out, unlocks. Returns `std::optional<T>` so the caller knows if anything was there.

### 2. OrderProcessor
New class that owns the matching engine, order book, serializer, and the order-to-client routing map. Runs a spin loop on its own thread, popping from the inbound queue and pushing serialized responses to the outbound queue.

### 3. ExchangeServer simplified to pure I/O
Removed `handle_message`, matching engine, order book, and serializer from `ExchangeServer`. It now only reads from sockets, pushes to the inbound queue, pops from the outbound queue, and writes to sockets.

### 4. Non-blocking epoll_wait
Changed `epoll_wait` timeout from `-1` (block forever) to `0` (return immediately). Without this, the I/O thread blocks in `epoll_wait` while responses sit in the outbound queue waiting to be drained. The client is waiting for a response before sending the next order, but the I/O thread is waiting for events that won't come until it sends that response. Classic deadlock.

## Results

| Configuration | 1 Client | 5 Clients |
|---|---|---|
| Single-threaded + write batching | 50,617 | 152,449 |
| Multithreaded, mutex queue | **59,941** | **131,764** |

**Single-client improved 18%.** Pipelining works. The I/O thread reads the next message while the processing thread matches the previous one.

**Multi-client regressed 14%.** Mutex contention kills the gains. The processing thread's spin loop calls `try_pop()` millions of times per second, locking and unlocking the mutex even when the queue is empty.

**`perf report` (1 client):**
```text
  66.87%  libc.so.6   [.] pthread_mutex_lock
  19.75%  libc.so.6   [.] __GI___pthread_mutex_unlock
   5.08%  exchange    [.] OrderProcessor::run()
```

**`perf report` (5 clients):**
```text
  61.57%  libc.so.6   [.] pthread_mutex_lock
  21.15%  libc.so.6   [.] __GI___pthread_mutex_unlock
   6.55%  exchange    [.] OrderProcessor::run()
```

Server instructions jumped to 302 billion (1 client), up from 1.8 billion in the single-threaded version. Hundreds of billions of empty lock/unlock cycles doing absolutely nothing.

## Next Steps
The architecture is correct and the single-client improvement proves pipelining works. The bottleneck is entirely the mutex queue. Adding an atomic size check before locking (skip the mutex when the queue is empty) or replacing it with a lock-free ring buffer would eliminate the contention eating 85%+ of CPU time.