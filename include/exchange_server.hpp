#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "message.hpp"
#include <vector>
#include <sys/epoll.h>
#include "client_state.hpp"
#include "inbound_message.hpp"
#include "outbound_message.hpp"
#include "thread_safe_queue.hpp"
#include "spsc_ring_buffer.hpp"



class ExchangeServer{
    public:
        ExchangeServer(uint16_t port, SPSCRingBuffer<InboundMessage, 8192>& iq, SPSCRingBuffer<OutboundMessage, 8192>& oq);

        void start();
        void run();

    private:

        void accept_client();
        void handle_read(int fd);
        void try_send(int fd);
        void handle_write(int fd);
        void disconnect_client(int fd);
        //helper for setting epoll listening events
        void set_epoll_write(int fd, bool enable);
        void flush_pending_writes();
        void drain_outbound_queue();
        int server_fd_;
        int epoll_fd_;
        //index into vector using fd
        std::vector<ClientState> clients_;
        //fds who are waiting to be sent
        std::vector<int> pending_writes_;
        //port where we listen for connections
        uint16_t port_;
        // ThreadSafeQueue<InboundMessage>& inbound_queue_;
        // ThreadSafeQueue<OutboundMessage>& outbound_queue_;
        SPSCRingBuffer<InboundMessage, 8192>& inbound_queue_;
        SPSCRingBuffer<OutboundMessage, 8192>& outbound_queue_;

};