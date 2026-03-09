
#include "exchange_server.hpp"
#include "message.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>

ExchangeServer::ExchangeServer(uint16_t port, OrderBook& order_book, MatchingEngine& matching_engine, Serializer& serializer)
    : port_{port}
    , order_book_{order_book}
    , matching_engine_{matching_engine}
    ,serializer_{serializer}
    {}


void ExchangeServer::start(){
    //SOCK_STREAM is TCP
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd_ == -1) throw std::runtime_error("socket creation failed");
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    //htons does conversion from port to network byte order, taking into account endianess
    address.sin_port = htons(port_);
    //cast the sockaddr_in to sockaddr, compatible with C API
    int bind_result = bind(server_fd_, (sockaddr*)&address, sizeof(address));
    if(bind_result == -1) throw std::runtime_error("bind failed");
    
    //5 = backlog, how many pending connections to queue
    int listen_result = listen(server_fd_, 5);
    if(listen_result == -1) throw std::runtime_error("listen failed");

}

void ExchangeServer::run(){

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    client_fd_ = accept(server_fd_, (sockaddr*)&client_addr, &client_len);
    if (client_fd_ == -1) throw std::runtime_error("accept failed");
    //here the program waits for a client to connect, blocking
    uint8_t header[5];

    while(true){
        //check if read_exact returns error
        //this also runs the function, so on success will copy 5 bytes into the header buffer
        if(!read_exact(client_fd_, header, 5)) break;

        //first byte is the message type, cast the first byte to the message type
        MessageType type = static_cast<MessageType>(header[0]);

        //remaining 4 bytes are the payload length
        uint32_t payload_length;
        //copy into payload_length, offset of 1 the first byte wass the message type
        //NOTE: we dont pass &header, because arrays already decay to pointers, where as other types do not
        std::memcpy(&payload_length, header + 1, sizeof(uint32_t));

        //now we know the size of the payload, we can read the payload itself
        std::vector<uint8_t> payload(payload_length);
        //payload.data(), returns pointer to the data segment of the vector
        if(!read_exact(client_fd_, payload.data(), payload_length)) break;

        //construct and handle the message
        Message message{
            .type = type,
            .payload = payload
        };
        handle_message(message);

    }
    close(client_fd_);



}

void ExchangeServer::handle_message(const Message& message){
/*
enum class MessageType : uint8_t {
    NEW_ORDER = 1,
    CANCEL_ORDER = 2,
    ORDER_ACK = 3,
    REJECT = 4,
    TRADE = 5
};
*/
    switch(message.type){
        case MessageType::NEW_ORDER: {
            //get the id outside the try, so we have it in scope in the catch
            uint64_t raw_id;
            //id is at the start of the payload
            std::memcpy(&raw_id, message.payload.data(), sizeof(uint64_t));
            try {
                Order order = serializer_.deserialize_order(message);
                //order received, send an ACK back
                Acknowledgement ack{
                    .id = order.id()
                };
                Message ack_msg = serializer_.serialize_acknowledgement(ack);
                send_message(client_fd_, ack_msg);
                //send the order to the matching engine
                std::vector<Trade> trades = matching_engine_.match(order);
                //process any trades made, and send trade messages
                for (const Trade& trade : trades){
                    Message trade_msg = serializer_.serialize_trade(trade);
                    send_message(client_fd_, trade_msg);
                }
            
            }catch(const std::exception& e){
                Rejection rejection{
                    .id = raw_id,
                    .reason = std::string(e.what())
                };
                Message rejection_msg = serializer_.serialize_rejection(rejection);
                send_message(client_fd_, rejection_msg);
                break;
            }
            
            break;
        }
        case MessageType::CANCEL_ORDER: {

            uint64_t raw_id;
            std::memcpy(&raw_id, message.payload.data(), sizeof(uint64_t));
            try {
                uint64_t id = serializer_.deserialize_cancel(message);
                Acknowledgement cancel_ack{
                    .id = id
                };
                Message cancel_ack_msg = serializer_.serialize_cancel_ack(cancel_ack);
                order_book_.cancel_order(id);
                send_message(client_fd_, cancel_ack_msg);

            }catch(const std::exception& e){
                Rejection rejection{
                    .id = raw_id,
                    .reason = std::string(e.what())
                };
                Message rejection_msg = serializer_.serialize_rejection(rejection);
                send_message(client_fd_, rejection_msg);
                break;
            }
            break;
        }
        default:
            throw std::runtime_error("unknown message type");
            break;
    }
}

void ExchangeServer::send_message(int fd, const Message& message){
    uint8_t type = static_cast<uint8_t>(message.type);
    //send(file descriptor, source, size, protocol)
    send(fd, &type, 1, 0);
    uint32_t length = message.payload.size();
    send(fd, &length, sizeof(uint32_t), 0);
    send(fd, message.payload.data(), length, 0);
}

bool ExchangeServer::read_exact(int fd, uint8_t* buffer, size_t n){
    size_t total_read = 0;
    while(total_read < n){
        //recv can return -1, so we need signed version of size_t, ssize_t
        ssize_t bytes_read = recv(fd, buffer + total_read, n - total_read, 0);
        if(bytes_read < 0) return false;
        total_read += bytes_read;
    }
    //if all bytes read
    return true;
}

