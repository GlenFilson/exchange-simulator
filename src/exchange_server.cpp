
#include "exchange_server.hpp"
#include "message.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include "network_utils.hpp"

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
    while(true){
        Message message;
        if(!read_message(client_fd_, message)) break;
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
                //send the order to the matching engine
                std::vector<Trade> trades = matching_engine_.match(order);
                //process any trades made, and send trade messages
                for (const Trade& trade : trades){
                    Message trade_msg = serializer_.serialize_trade(trade);
                    send_message(client_fd_, trade_msg);
                }
                //only ack order after any trades have been sent
                Acknowledgement ack{
                    .id = order.id()
                };
                Message ack_msg = serializer_.serialize_acknowledgement(ack);
                send_message(client_fd_, ack_msg);
            
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



