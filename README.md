# Exchange Simulator

High-performance simulated exchange built from scratch in C++. Accepts orders over TCP sockets, matches with price-time priority, and routes fills to clients. Epoll I/O, multithreaded processing pipeline, lock-free queues, binary protocol. Profiled and optimized from 12 to 1.1M+ ops/sec.

## Architecture

```mermaid
graph LR
    Clients[Concurrent Clients] -->|TCP Non-blocking| Server[Exchange Server\nI/O Thread]
    Server -->|SPSC Ring Buffer| Processor[Order Processor\nProcessing Thread]
    Processor --> ME[Matching Engine]
    Processor --> OB[Order Book]
    ME --> OB
    Processor -->|SPSC Ring Buffer| Server
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
    class SPSCRingBuffer~T~ {
        -array~T~ buffer_
        -atomic~size_t~ head_
        -atomic~size_t~ tail_
        +try_push(T item) bool
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
    ExchangeServer --> SPSCRingBuffer : pushes inbound\npops outbound
    OrderProcessor --> SPSCRingBuffer : pops inbound\npushes outbound
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

1. [TCP_NODELAY + Single Send Buffer](docs/improvements/01-tcp-nodelay.md) - 12 -> 46K ops/sec (3,800x)
2. [Pre-allocated Buffers](docs/improvements/02-preallocated-buffers.md) - 46K -> 50K ops/sec, 83% instruction reduction
3. [Epoll Multiplexing](docs/improvements/03-epoll-concurrency.md) - Unlocked multi-client support, 134K combined ops/sec
4. [Flat Arrays & Write Batching](docs/improvements/04-flat-arrays-write-batching.md) - 134K -> 152K ops/sec, eliminated epoll_ctl and hash map overhead
5. [Multithreaded I/O + Processing](docs/improvements/05-multithreaded-architecture.md) - Separated I/O from matching, 18% single-client gain
6. [Lock-free SPSC Ring Buffer](docs/improvements/06-lock-free-ring-buffer.md) - Eliminated mutex contention, explored queue bottlenecks
7. [Client Pipelining & Read Batching](docs/improvements/07-client-pipelining-read-batching.md) - 153K single-client, 750K with 5 clients

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

- Multicast UDP market data
- Shared memory ring buffers
- JSON serializer
- Unit testing