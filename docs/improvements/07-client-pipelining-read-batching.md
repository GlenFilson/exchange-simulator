# Client Pipelining & Read Batching

## The Problem
After implementing multithreading and a lock-free ring buffer, throughput plateaued at ~140K ops/sec with 5 clients. The processing thread sat idle 90% of the time. The queue optimizations barely moved throughput. Something else was limiting the system.

## Investigation

### Matching engine benchmark
Benchmarking the matching engine in isolation (no networking) revealed it could process **9.4 million orders per second**. The server was only achieving 140K through TCP. Networking was eating 98.5% of the matching engine's potential.

### Synchronous client bottleneck
The client sent one order, waited for the ACK, then sent the next. One order in flight at a time. With a localhost round-trip of ~20-30 microseconds, the theoretical max is ~40K orders/sec per client. The server spent most of its time waiting for the next message to arrive.

```
send order → wait for response → send next order → wait for response
```

The processing thread was fast enough. The I/O thread was fast enough. The clients just weren't feeding them fast enough.

## The Fix

### 1. Client pipelining
Changed the client to send orders in batches without waiting for responses. Send N orders, then drain all N responses.

```cpp
for (int batch = 0; batch < NUM_ORDERS / BATCH_SIZE; batch++){
    // blast a batch without waiting
    for (int i = 0; i < BATCH_SIZE; i++){
        Order order = order_simulator_.generate_order();
        msg_buffer_.clear();
        serializer_.serialize_order(order, msg_buffer_);
        send(socket_fd_, msg_buffer_.data(), msg_buffer_.size(), 0);
    }
    // now drain all responses
    int acks_received = 0;
    while (acks_received < BATCH_SIZE){
        if (!read_message(socket_fd_, response)) break;
        if (response.type == MessageType::ORDER_ACK || response.type == MessageType::REJECT)
            acks_received++;
    }
}
```

This immediately jumped single-client throughput from 63K to 152K. The server's inbound queue actually had work in it for the first time.

### 2. Read batching
The I/O thread's `handle_read` previously read one message per epoll event. If a client had 100 messages buffered in the kernel, the server would read one, return to the event loop, call epoll_wait, wake up, read another. 100 event loop cycles for 100 messages.

Wrapped `handle_read` in a `while(true)` loop that keeps reading until EAGAIN. One epoll event can now process hundreds of messages from the same client in a single pass.

```cpp
void ExchangeServer::handle_read(int fd){
    while(true){
        // read header...
        // read payload...
        // push to queue
        if(!inbound_queue_.try_push(i_message)) return;  // queue full, come back later
        // reset state, loop back to read next message
    }
    // only exits on EAGAIN, disconnect, or full queue
}
```

This required careful handling of the queue-full case. If the push fails, the client state stays intact so the message can be retried on the next event loop cycle. The payload is copied (not moved) into the InboundMessage so the buffer survives a failed push.

### 3. Ring buffer sizing
Increased ring buffer capacity from 1024 to 8192 slots. With pipelined clients sending thousands of orders in bursts, the smaller buffer filled up frequently, causing the I/O thread to bail out of read batching early.

## Results

| Configuration | 1 Client | 5 Clients |
|---|---|---|
| SPSC ring buffer, synchronous client | 63,763 | 139,194 |
| Pipelined client | 153,424 | 186,876 |
| Pipelined client + read batching | **153,424** | **749,967** |

* Single-client: 63K → 153K (**2.4x**)
* Multi-client: 139K → 750K (**5.4x**)
* Per-client throughput stays at ~150K up to 5 clients. Linear scaling.
* Server completes 5M orders across 5 clients in 6.7 seconds.

The single-client number didn't change with read batching because with one client there's nothing to batch. The gain is entirely multi-client: reading many messages from each client before moving to the next one, instead of round-robin one message at a time.

## Next Steps
Profiling the current setup will reveal where the new bottleneck is. Potential areas to investigate: multiple I/O threads, kernel bypass (io_uring/DPDK), further reducing copies on the queue path, and cache line alignment on the ring buffer.