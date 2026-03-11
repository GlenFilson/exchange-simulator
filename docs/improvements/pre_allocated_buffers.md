

## ExchangeServer::run()
```cpp
while(true){
    Message message;//message instantiated every loop
    if(!read_message(client_fd_, message)) break;
    handle_message(message);
}
```
### Changed to:
```cpp
Message message;//message reused for every loop
while(true){
    if(!read_message(client_fd_, message)) break;
    handle_message(message);
}
```


## read_message()
```cpp
bool read_message(int fd, Message& message){
    uint8_t header[5];
    //check if read_exact returns error
    //this also runs the function, so on success will copy 5 bytes into the header buffer
    if(!read_exact(fd, header, 5)) return false;
    //first byte is the message type, cast the first byte to the message type
    message.type = static_cast<MessageType>(header[0]);
    //remaining 4 bytes are the payload length
    uint32_t payload_length;
    //copy into payload_length, offset of 1 the first byte wass the message type
    //NOTE: we dont pass &header, because arrays already decay to pointers, where as other types do not
    std::memcpy(&payload_length, header + 1, sizeof(uint32_t));
    //now we know the size of the payload, we can set the size of the payload vector in the message
    message.payload.resize(payload_length);
    //payload.data(), returns pointer to the data segment of the vector
    if(!read_exact(fd, message.payload.data(), payload_length)) return false;
    return true;
}
```

## send_message()
```cpp
void send_message(int fd, const Message& message){
    uint8_t type = static_cast<uint8_t>(message.type);
    uint32_t length = message.payload.size();
    //store all in a single message, rather than sending each segment individually
    //each send triggers a syscall, so best to minimise by packing all data into 1 message
    std::vector<uint8_t> buffer(1+sizeof(uint32_t) + length);
    buffer[0] = type;
    std::memcpy(buffer.data() + 1, &length, sizeof(uint32_t));
    std::memcpy(buffer.data() + 1 + sizeof(uint32_t), message.payload.data(), length);
    //send(file descriptor, source, size, protocol)
    send(fd, buffer.data(), buffer.size(), 0);
}
```