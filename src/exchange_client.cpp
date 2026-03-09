
#include "exchange_client.hpp"
#include "message.hpp"
#include <arpa/inet.h>
#include "network_utils.hpp"
#include <iostream>
#include "trade.hpp"
#include "acknowledgement.hpp"
#include "rejection.hpp"

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

}

void ExchangeClient::run(){
    int NUM_ORDERS = 100;
    for (int i = 0; i < NUM_ORDERS; i++){
        Order order = order_simulator_.generate_order();
        // order.toString();
        Message order_msg = serializer_.serialize_order(order);
        send_message(socket_fd_, order_msg);
        while(true){
            Message response;
            if(!read_message(socket_fd_, response)) break;
            if(response.type == MessageType::ORDER_ACK){
                Acknowledgement ack = serializer_.deserialize_acknowledgement(response);
                std::cout << "ACK order: " << ack.id << "\n";
                break;
            }else if (response.type == MessageType::TRADE){
                Trade trade = serializer_.deserialize_trade(response);
                std::cout << "Trade: " << trade.quantity << " @ " << trade.price << "\n";
                //dont break on trade, trade does not mean the entire order has been filled
            }else if(response.type == MessageType::REJECT){
                Rejection rejection = serializer_.deserialize_rejection(response);
                std::cout << "Rejected: " << rejection.reason << "\n";
                break;
            }
        }
    }
    close(socket_fd_);
}
