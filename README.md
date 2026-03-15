# Exchange Simulator

High-performance simulated exchange built from scratch in C++. Order book, matching engine, binary serialization, TCP networking, multithreaded I/O and processing pipeline.

## Architecture

```mermaid
graph LR
    Clients[Concurrent Clients] -->|TCP Non-blocking| Server[Exchange Server I/O Thread]
    Server -->|inbound queue| Processor[Order Processor Processing Thread]
    Processor --> ME[Matching Engine]
    Processor --> OB[Order Book]
    ME --> OB
    Processor -->|outbound queue| Server
    Server -->|TCP| Clients
```

## Components

```mermaid
classDiagram
    class OrderBook {
        -map bids_
        -map asks_
        -unordered_map order_map_
        +add_order(const Order&)
        +cancel_order(uint64_t)
        +best_bid() optional
        +best_ask() optional
        +print_depth(ostream&, int)
    }
    class MatchingEngine {
        -OrderBook& order_book_
        +match(const Order&) vector~Trade~
    }
    class ClientState {
        +ReadPhase read_phase
        +uint32_t payload_length
        +vector~uint8_t~ read_buffer
        +vector~uint8_t~ write_buffer
    }
    class ThreadSafeQueue~T~ {
        -queue~T~ queue_
        -mutex mutex_
        +push(T item)
        +try_pop() optional~T~
    }
    class OrderProcessor {
        -MatchingEngine& matching_engine_
        -OrderBook& order_book_
        -Serializer& serializer_
        -unordered_map order_to_client_fd_
        +run()
        +process(InboundMessage&)
    }
    class Serializer {
        <<abstract>>
        +serialize_order()*
        +deserialize_order()*
        +serialize_trade()*
        +deserialize_trade()*
    }
    class BinarySerializer {
        +serialize_order()
        +deserialize_order()
        +serialize_trade()
        +deserialize_trade()
    }
    class ExchangeServer {
        -int epoll_fd_
        -vector~ClientState~ clients_
        +start()
        +run()
        +handle_read(int)
        +handle_write(int)
    }
    class ExchangeClient {
        -Serializer& serializer_
        -OrderSimulator& simulator_
        +connect()
        +run()
    }

    ExchangeServer *-- ClientState : manages
    ExchangeServer --> ThreadSafeQueue : pushes inbound, pops outbound
    OrderProcessor --> ThreadSafeQueue : pops inbound, pushes outbound
    OrderProcessor --> MatchingEngine
    OrderProcessor --> OrderBook
    OrderProcessor --> Serializer
    MatchingEngine --> OrderBook : friend access
    Serializer <|-- BinarySerializer
    ExchangeClient --> Serializer
```

## Wire Format

```
┌──────────────┬──────────────────┬─────────────┐
│ Type (1 byte)│ Length (4 bytes) │ Payload (N) │
└──────────────┴──────────────────┴─────────────┘
```

## Optimizations

1. [TCP_NODELAY + Single Send Buffer](docs/improvements/nagles_algorithm.md) - 12 -> 46K ops/sec (3,800x)
2. [Pre-allocated Buffers](docs/improvements/pre_allocated_buffers.md) - 46K -> 50K ops/sec, 83% instruction reduction
3. [Epoll Multiplexing](docs/improvements/epoll_concurrency.md) - Unlocked multi-client support, 134K combined ops/sec
4. [Flat Arrays & Write Batching](docs/improvements/flat_arrays_write_batching.md) - 134K -> 152K ops/sec, eliminated epoll_ctl and hash map overhead
5. [Multithreaded I/O + Processing](docs/improvements/multithreading_mutex_queue.md) - Separated I/O from matching, 18% single-client gain, mutex contention under load

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .

# Terminal 1
./exchange

# Terminal 2
./client N  # number of concurrent clients (optional, default=1)
```

## Future Work

- Lock-free ring buffer queue
- Multicast UDP market data
- Shared memory ring buffers
- JSON serializer
- Unit testing