#pragma once

#include "orderbook.hpp"
#include "matching_engine.hpp"
#include "serializer.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "message.hpp"


class ExchangeServer{

    public:
        ExchangeServer(uint16_t port, OrderBook& order_book, MatchingEngine& matching_engine, Serializer& serializer);

        void start();
        void run();

    private:
        void handle_message(const Message& message);
        int server_fd_;
        int client_fd_;
        uint16_t port_;
        OrderBook& order_book_;
        MatchingEngine& matching_engine_;
        //base class reference, can take any derived class serializer
        Serializer& serializer_;

};