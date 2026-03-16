# Lock-free SPSC Ring Buffer

## The Problem
After splitting I/O and processing onto separate threads, profiling showed 85% of CPU time on `pthread_mutex_lock` and `pthread_mutex_unlock`. The processing thread's spin loop called `try_pop()` millions of times per second, and every call locked and unlocked the mutex even when the queue was empty.

**`perf report` (mutex queue):**
```text
  66.87%  libc.so.6   [.] pthread_mutex_lock
  19.75%  libc.so.6   [.] __GI___pthread_mutex_unlock
   5.08%  exchange    [.] OrderProcessor::run()
```

Server instructions hit 302 billion for 1M orders. The actual work was maybe 2 billion. The rest was empty lock/unlock cycles.

## Investigation

### Step 1: Atomic size check
Added an `std::atomic<size_t>` counter to the queue. `try_pop` checks the atomic before locking. If the queue is empty (which is 99.99% of the time in the spin loop), it returns immediately without touching the mutex. One atomic load instead of lock + check + unlock.

```cpp
std::optional<T> try_pop(){
    // no lock needed for empty check
    if(size_.load(std::memory_order_acquire) == 0) return std::nullopt;
    std::lock_guard<std::mutex> lock(mutex_);
    // ... pop under lock
}
```

This dropped mutex from 85% to under 1% of CPU. But throughput only improved modestly because the mutex was never the bottleneck on the data path, only on the empty-check path.

### Step 2: SPSC ring buffer
Replaced the mutex queue entirely with a lock-free single-producer single-consumer ring buffer. Fixed-size circular array with two atomic indices: one for the writer, one for the reader. No mutex at all.

```cpp
template <typename T, size_t Capacity>
class SPSCRingBuffer {
    bool try_push(T item){
        size_t head = head_.load(std::memory_order_relaxed);   // only I write head
        size_t tail = tail_.load(std::memory_order_acquire);   // see reader's progress
        if(head - tail == Capacity) return false;              // full
        buffer_[head & (Capacity - 1)] = std::move(item);
        head_.store(head + 1, std::memory_order_release);      // publish data
        return true;
    }
    std::optional<T> try_pop(){
        size_t tail = tail_.load(std::memory_order_relaxed);   // only I write tail
        size_t head = head_.load(std::memory_order_acquire);   // see writer's progress
        if(tail == head) return std::nullopt;                  // empty
        T item = std::move(buffer_[tail & (Capacity - 1)]);
        tail_.store(tail + 1, std::memory_order_release);      // free slot
        return item;
    }
};
```

Key properties:
- Writer only writes `head_`, reader only writes `tail_`. No shared writes, minimal cache contention.
- Capacity is a power of 2, so `% Capacity` becomes `& (Capacity - 1)` (bitwise AND instead of division).
- `memory_order_release` on writes ensures data is visible before the index update. `memory_order_acquire` on reads ensures we see the data after seeing the index update.
- `memory_order_relaxed` for reading your own index since no other thread writes to it.

## Results

| Configuration | 1 Client | 5 Clients |
|---|---|---|
| Mutex queue | 59,941 | 131,764 |
| Atomic size check | 63,807 | 140,043 |
| SPSC ring buffer | 63,763 | 139,194 |

The ring buffer matched the atomic check version. Throughput didn't improve because the bottleneck wasn't the queue at all. The synchronous client only had one order in flight at a time, starving the server of work. The processing thread was idle 90% of the time waiting for the next message regardless of how fast the queue was.

## Next Steps
The queue is no longer the bottleneck. The synchronous client's round-trip latency is the limiting factor. Pipelining orders (sending many without waiting for responses) should keep the queue full and let the processing thread work continuously.