# Pre-allocated Buffers

## Evidence

Profiling showed `std::vector::_M_default_append` at 4.14% — top non-kernel hotspot. Called from `read_message` resizing the payload vector on every message. `malloc` and vector destructor also in top 30.

## Analysis

`Message` created inside the loop — new payload vector allocated and freed every iteration. `send_message` also allocating a new buffer every call. Millions of malloc/free pairs for 1M orders.

## Fix

### ExchangeServer::run()
```cpp
while(true){
    Message message;//message instantiated every loop
    if(!read_message(client_fd_, message)) break;
    handle_message(message);
}
```
#### Changed to:
```cpp
Message message;//message reused for every loop
while(true){
    if(!read_message(client_fd_, message)) break;
    handle_message(message);
}
```

### send_message()
```cpp
void send_message(int fd, const Message& message){
    uint8_t type = static_cast<uint8_t>(message.type);
    uint32_t length = message.payload.size();
    std::vector<uint8_t> buffer(1+sizeof(uint32_t) + length);
    buffer[0] = type;
    std::memcpy(buffer.data() + 1, &length, sizeof(uint32_t));
    std::memcpy(buffer.data() + 1 + sizeof(uint32_t), message.payload.data(), length);
    send(fd, buffer.data(), buffer.size(), 0);
}
```
#### Changed to:
```cpp
void send_message(int fd, const Message& message, std::vector<uint8_t>& buffer){
    uint8_t type = static_cast<uint8_t>(message.type);
    uint32_t length = message.payload.size();
    buffer.resize(1+sizeof(uint32_t) + length);
    buffer[0] = type;
    std::memcpy(buffer.data() + 1, &length, sizeof(uint32_t));
    std::memcpy(buffer.data() + 1 + sizeof(uint32_t), message.payload.data(), length);
    send(fd, buffer.data(), buffer.size(), 0);
}
```

Same changes applied on client side.

## Results

- Instructions: 10.3B → 1.7B server, 6.6B → 1.05B client
- User time: 1.39s → 0.65s server, 1.24s → 0.78s client
- Throughput: 46,728 → 50,643 orders/sec (**8.4%**)
- Throughput gain limited by syscall/network bottleneck — sys time barely changed (8.48s → 8.10s)