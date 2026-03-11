#pragma once
#include "order_simulator.hpp"
#include "serializer.hpp"
#include "message.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <vector>
class ExchangeClient {
    public:
        ExchangeClient(const std::string& host, uint16_t port, Serializer& serializer, OrderSimulator& order_simulator);

        void connect();
        void run();
        
    private:
        int socket_fd_;
        std::string host_;
        uint16_t port_;
        std::vector<uint8_t> msg_buffer_;
        Serializer& serializer_;
        OrderSimulator& order_simulator_; 
};