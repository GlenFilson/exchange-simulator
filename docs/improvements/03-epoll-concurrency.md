# Non-blocking I/O & Epoll Multiplexing

## The Problem
The exchange was strictly single-threaded and used blocking `recv()` sockets. It could process one client quickly, but inherently serialized connections. It was incapable of simulating an exchange with multiple concurrent clients.

## Investigation
Using a thread-per-client architecture introduces prohibitive OS context-switching overhead at scale. To achieve high concurrency within a single thread, the networking layer required an asynchronous, event-driven architecture based on I/O multiplexing.

## The Fix
1. Converted the main accept/read/write loops to a Linux `epoll` multiplexer.
2. Flagged all server and client sockets with `O_NONBLOCK` via standard `fcntl`.
3. Created a `ClientState` struct to track partial network reads (`HEADER` vs `PAYLOAD` phases) and maintain individual read/write buffers.
4. Implemented maker/taker routing logic to notify multiple clients concurrently (both parties of a matched trade) via `epoll_ctl` `EPOLLOUT` events.

**Architecture Shift:**
```cpp
// non-blocking socket setup
int flags = fcntl(client_fd, F_GETFL, 0);
fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

// Epoll Registration
epoll_event ev{};
ev.events = EPOLLIN;
ev.data.fd = client_fd;
epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev);
```

## Results
* **1 Client Throughput:** 50,643 ops/sec $\rightarrow$ 47,801 ops/sec. 
  *(Note: A ~5.6% regression is expected here due to the abstraction cost of `epoll_ctl` syscalls, hash map lookups, and state machine branching compared to a raw blocking loop).*
* **5 Client Throughput:** Blocked completely $\rightarrow$ **134,735 combined ops/sec**. 

## Next Steps
The new architecture successfully scales horizontally. Profiling the new setup under concurrent load (`perf record -e cpu-clock`) reveals massive inefficiencies in the event loop routing that can be optimized next:

**`perf report` excerpt:**
```text
 29.36%  exchange  libc.so.6   [.] epoll_ctl
  3.19%  exchange  exchange    [.] std::_Hashtable<unsigned long, std::pair<...
```
1. `epoll_ctl` accounts for nearly 30% of all CPU time due to modifying the `EPOLLOUT` flag for every single message.
2. Hash maps (`std::_Hashtable`) account for ~11% of CPU time due to looking up the `ClientState` by file descriptor on every network event.