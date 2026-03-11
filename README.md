# Exchange Simulator

Simulated exchange built from scratch in C++. Order book, matching engine, binary serialization, TCP networking.

## Architecture

```mermaid
graph LR
    Client[Client] -->|TCP| Server[Exchange Server]
    Server --> Serializer[Binary Serializer]
    Serializer --> Handler[Message Handler]
    Handler --> ME[Matching Engine]
    Handler --> OB[Order Book]
    ME --> OB
    Handler -->|ACK / Trade / Reject| Serializer
    Serializer -->|TCP| Client
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
        -OrderBook& order_book_
        -MatchingEngine& engine_
        -Serializer& serializer_
        +start()
        +run()
    }
    class ExchangeClient {
        -Serializer& serializer_
        -OrderSimulator& simulator_
        +connect()
        +run()
    }
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

## Performance
## Optimizations

1. [TCP_NODELAY + Single Send Buffer](docs/improvements/nagles_algorithm.md) — 12 → 46K orders/sec (~3800x)
2. [Pre-allocated Buffers](docs/improvements/preallocated-buffers.md) — in progress

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
./client
```

## Future Work

- Multithreading
- Multicast UDP market data
- Shared memory ring buffers
- JSON serializer
- Pre-allocated buffers
- Unit testing
- Unit testing