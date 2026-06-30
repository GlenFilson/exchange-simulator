
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
    int BATCH_SIZE = 10000;
    auto start = std::chrono::high_resolution_clock::now();
    Message response;
    for (int batch = 0; batch < NUM_ORDERS / BATCH_SIZE; batch++){
        // send a full batch without waiting
        for (int i = 0; i < BATCH_SIZE; i++){
            Order order = order_simulator_.generate_order();
            msg_buffer_.clear();
            serializer_.serialize_order(order, msg_buffer_);
            send(socket_fd_, msg_buffer_.data(), msg_buffer_.size(), 0);
        }
        // now drain all responses for this batch
        // int acks_received = 0;
        // while (acks_received < BATCH_SIZE){
        //     if (!read_message(socket_fd_, response)) break;
        //     if (response.type == MessageType::ORDER_ACK || response.type == MessageType::REJECT){
        //         acks_received++;
        //     }
        //     // trades are consumed but don't count toward acks
        // }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    std::cout << NUM_ORDERS << " orders in " << ms << "ms\n";
    std::cout << (NUM_ORDERS * 1000 / ms) << " orders/sec\n";
    close(socket_fd_);
}
