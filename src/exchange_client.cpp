
#include "exchange_client.hpp"
#include "message.hpp"
#include <arpa/inet.h>
#include "network_utils.hpp"
#include <iostream>
#include "trade.hpp"
#include "acknowledgement.hpp"
#include "rejection.hpp"
#include <chrono>
//add this to allow disabling Nagles algorithm, throttling TCP packets
#include <netinet/tcp.h>

ExchangeClient::ExchangeClient(const std::string& host, uint16_t port, Serializer& serializer, OrderSimulator& order_simulator)
    : host_{host}
    , port_{port}
    , serializer_{serializer}
    , order_simulator_{order_simulator}
    {}

void ExchangeClient::connect(){
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd_ == -1) throw std::runtime_error("socket creation failed");

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port_);
    inet_pton(AF_INET, host_.c_str(), &address.sin_addr);
    
    int connect_result = ::connect(socket_fd_, (sockaddr*)&address, sizeof(address));
    if(connect_result == -1) throw std::runtime_error("failed to connect");
    int flag = 1;
    setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
}

void ExchangeClient::run(){
    int NUM_ORDERS = 1000000;
    auto start = std::chrono::high_resolution_clock::now();
    Message response;
    Message order_msg;
    for (int i = 0; i < NUM_ORDERS; i++){
        Order order = order_simulator_.generate_order();
        // order.toString();
        Message order_msg = serializer_.serialize_order(order);
        send_message(socket_fd_, order_msg, msg_buffer_);
        while(true){
            if(!read_message(socket_fd_, response)) break;
            if(response.type == MessageType::ORDER_ACK){
                Acknowledgement ack = serializer_.deserialize_acknowledgement(response);
                // std::cout << "ACK order: " << ack.id << "\n";
                break;
            }else if (response.type == MessageType::TRADE){
                Trade trade = serializer_.deserialize_trade(response);
                // std::cout << "Trade: " << trade.quantity << " @ " << trade.price << "\n";
                //dont break on trade, trade does not mean the entire order has been filled
            }else if(response.type == MessageType::REJECT){
                Rejection rejection = serializer_.deserialize_rejection(response);
                // std::cout << "Rejected: " << rejection.reason << "\n";
                break;
            }
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    std::cout << NUM_ORDERS << " orders in " << ms << "ms\n";
    std::cout << (NUM_ORDERS * 1000 / ms) << " orders/sec\n";
    close(socket_fd_);
}
