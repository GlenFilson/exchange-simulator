#pragma once

#include "orderbook.hpp"
#include "matching_engine.hpp"
#include "serializer.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "message.hpp"
#include <vector>
#include <sys/epoll.h>
#include "client_state.hpp"


class ExchangeServer{
    public:
        ExchangeServer(uint16_t port, OrderBook& order_book, MatchingEngine& matching_engine, Serializer& serializer);

        void start();
        void run();

    private:
        void handle_message(int client_fd, const Message& message);
        void accept_client();
        void handle_read(int fd);
        void try_send(int fd);
        void handle_write(int fd);
        void disconnect_client(int fd);
        //helper for setting epoll listening events
        void set_epoll_write(int fd, bool enable);
        void flush_pending_writes();
        int server_fd_;
        int epoll_fd_;
        //tracks file descriptor to a client state
        // std::unordered_map<int, ClientState> clients_;
        //index into vector using fd
        std::vector<ClientState> clients_;
        //fds who are waiting to be sent
        std::vector<int> pending_writes_;
        //order id to clients file descriptor, so we know what clients to broadcast trade messages to
        std::unordered_map<uint64_t, int> order_to_client_fd_;
        //port where we listen for connections
        uint16_t port_;
        // std::vector<uint8_t> msg_buffer_; message buffer no longer needed, stored per client
        OrderBook& order_book_;
        MatchingEngine& matching_engine_;
        //base class reference, can take any derived class serializer
        Serializer& serializer_;

};