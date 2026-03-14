# Exchange Simulator

High-performance simulated exchange built from scratch in C++. Order book, matching engine, binary serialization, TCP networking.

## Architecture

```mermaid
graph LR
    Clients[Concurrent Clients] -->|TCP Non-blocking|Server[Exchange Server]
    Server -->|epoll multiplexing| Serializer[Binary Serializer]
    Serializer --> Handler[Message Handler]
    Handler --> ME[Matching Engine]
    Handler --> OB[Order Book]
    ME --> OB
    Handler -->|Route Trade / Auth| Server
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
        -unordered_map clients_
        -OrderBook& order_book_
        -MatchingEngine& engine_
        -Serializer& serializer_
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
    
    ExchangeServer *-- ClientState : Manages
    MatchingEngine --> OrderBook : friend access
    Serializer <|-- BinarySerializer
    ExchangeServer --> MatchingEngine
    ExchangeServer --> OrderBook
    ExchangeServer --> Serializer
    ExchangeClient --> Serializer
```

<!-- **Order Book** — 

**Matching Engine** — 

**Serialization** — 

**Networking** —  -->

## Wire Format

```
┌──────────────┬──────────────────┬─────────────┐
│ Type (1 byte)│ Length (4 bytes) │ Payload (N) │
└──────────────┴──────────────────┴─────────────┘
```

<!-- ## Performance -->
## Optimizations
1. [TCP_NODELAY + Single Send Buffer](docs/improvements/nagles_algorithm.md) - 12 ops/sec -> 46K ops/sec (3,800x increase)
2. [Pre-allocated Buffers & Zero-Allocation Path](docs/improvements/pre_allocated_buffers.md) - 46K -> 50K ops/sec, 83% instruction reduction
3. [Non-blocking I/O & Epoll Multiplexing](docs/improvements/epoll_concurrency.md) - Unlocked horizontal scaling, 134K+ combined ops/sec

<!-- ## Design Decisions -->

<!-- Key tradeoffs: data structures, O(1) cancel, friend class, abstract serializer, TCP_NODELAY -->

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .

# Terminal 1
./exchange

# Terminal 2
./client N #number of concurrent clients(optional, default=1)
```

## Future Work

- Multithreading
- Multicast UDP market data
- Shared memory ring buffers
- JSON serializer
- Unit testing
