
#include "exchange_server.hpp"
#include "message.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include "network_utils.hpp"
#include <netinet/tcp.h>
#include <fcntl.h>
#include <cerrno>
#include <arpa/inet.h>
ExchangeServer::ExchangeServer(uint16_t port, SPSCRingBuffer<InboundMessage, 8192>& iq, SPSCRingBuffer<OutboundMessage, 8192>& oq)
    : port_{port}
    , inbound_queue_{iq}
    , outbound_queue_{oq}
    {}


void ExchangeServer::start(){
    //SOCK_STREAM is TCP
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    //lets the port be used without waiting 60 seconds between usages
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(server_fd_ == -1) throw std::runtime_error("socket creation failed");
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    //htons does conversion from port to network byte order, taking into account endianess
    address.sin_port = htons(port_);
    //cast the sockaddr_in to sockaddr, compatible with C API
    int bind_result = bind(server_fd_, (sockaddr*)&address, sizeof(address));
    if(bind_result == -1) throw std::runtime_error("bind failed");
    
    //5: backlog, how many pending connections are allowed to queue
    int listen_result = listen(server_fd_, 5);
    if(listen_result == -1) throw std::runtime_error("listen failed");

    int flags = fcntl(server_fd_, F_GETFL, 0); //get the current flags
    //add nonblocking socket to existing flags
    if (fcntl(server_fd_, F_SETFL, flags | O_NONBLOCK) == -1) throw std::runtime_error("failed to set socket to non blocking"); 
    epoll_fd_ = epoll_create1(0);
    if(epoll_fd_ == -1) throw std::runtime_error("epoll creation failed");
    //create an e poll event and populate its fields
    epoll_event ev{};
    ev.events = EPOLLIN; //the associated file is available for read operations
    /*
    data is simply what we want to be returned when the listend fd has an event
    this can be anything of:
    typedef union epoll_data {
        void *ptr;
        int fd;
        uint32_t u32;
        uint64_t u64;
    } epoll_data_t;
    meaning, we could create our own struct for metadata that we want returned on events
    then we simply store as data, a ptr to the struct. e.g.:
    struct Connection {
        int fd;
        Buffer buffer;
    };
    Connection* conn = new Connection{client_fd};
    ev.events = EPOLLIN;
    ev.data.ptr = conn;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
    */
    ev.data.fd = server_fd_;//what we want epoll to return on an event, we want the server_fd_ so we quickly know this is an event on server_fd_, we need to accept new connection
    //epoll_fd_: the epoll instance, EPOLL_CTL_ADD: add an entry of interest to the epoll instance
    if(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_fd_, &ev) == -1) throw std::runtime_error("failed to register server with epoll");

    int flag = 1;
    setsockopt(server_fd_, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    //up to 1024 file descriptors
    clients_.resize(1024);
}

void ExchangeServer::run(){
    epoll_event events[64]; //buffer for ready events (may need to alter size)
    while(true){
        drain_outbound_queue();
        flush_pending_writes();

        //n: number of events occured + written, events: where to write about any fds ready for reading, 64: number of events we can write to, 0: non-blocking
        int n = epoll_wait(epoll_fd_, events, 64, 0);
        for (int i = 0; i < n; i++){
            int fd = events[i].data.fd;//here we reaccess the data that we wanted returned on an event, that is the file descriptor
            if(fd == server_fd_){
                //any server event is a client trying to connect
                accept_client();
            }else{
                if(events[i].events & EPOLLIN){//bitwise and, check if the EPOLLIN flag is set in events (which is a bitfield of flags)
                //if the fd is readable, read it
                handle_read(fd);
                }
                if(events[i].events & EPOLLOUT){
                    //if the fd is writaable, write to it 
                    handle_write(fd);
                }
            }
        } 
    } 
}
void ExchangeServer::accept_client(){
    //instead of just accepting one client per call, loop through all clients in the queue and accept them all
    //this can optimise performance as we dont accept one client, go back to main loop, accept another client, etc.
    //we reduce the repeated epoll wakeups / events for events of the same socket (clients on server_fd_)
    while(true){
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd_, (sockaddr*)&client_addr, &client_len);
        if(client_fd == -1){
            //non-blocking accept can return EAGAIN when there are no connections left in the accept queue
            if(errno == EAGAIN || errno == EWOULDBLOCK) break;
            //if its an error that is not these cases, not acceptable, throw exception
            throw std::runtime_error("accept failed");
        }

        //need to set every client server to be non blocking
        int flags = fcntl(client_fd, F_GETFL, 0);
        if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) throw std::runtime_error("failed to set socket to non blocking"); 
        
        epoll_event ev{};
        ev.events = EPOLLIN;
        //NOTE: consider instead of registering fd, registering a ptr to the entry in the clients_ map
        //then we dont have to get client_fd, then lookup the client state in map, we could have direct access
        ev.data.fd = client_fd;
        if(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) == -1) throw std::runtime_error("failed to register client with epoll");
    
        int flag = 1;
        setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
        clients_[client_fd] = ClientState{};//map the client fd to default constructed ClientState
        clients_[client_fd].active = true;
    }
}

void ExchangeServer::handle_read(int fd){
    ClientState& client = clients_[fd];
    if(!client.active) return;
    while (true) {
        //read into header_buffer offset by the number of bytes we have already read
        //we want to read another 5-bytes_read, basically read until we have read 5 bytes, filled the buffer
        ssize_t n = recv(fd, client.recv_buffer + client.recv_bytes, ClientState::RECV_BUFFER_SIZE - client.recv_bytes, MSG_DONTWAIT);//non-blocking recv
        if(n > 0){
            client.recv_bytes += n;
        }else if(n == 0){//connection closed
            disconnect_client(fd);
            return;
        }else if(n == -1){
            //acceptable errors, just exit
            if(errno == EAGAIN || errno == EWOULDBLOCK) return;
            disconnect_client(fd);
            return;
            // throw std::runtime_error("error receiving client header");
        }
        //only begin parsing if we have at least a message header worth of bytes
        while (client.parse_offset + 5 <= client.recv_bytes) {
            MessageType type = static_cast<MessageType>(client.recv_buffer[client.parse_offset]);
            uint32_t payload_length;
            std::memcpy(&payload_length,client.recv_buffer + client.parse_offset + 1, sizeof(uint32_t));
            payload_length = ntohl(payload_length);
            //check if the rest of the message is available, the body, the remaining length
            //offset after reading the header (1 byte message type, 4 bytes uint32_t payload length)
            if (client.parse_offset + 5 + payload_length > client.recv_bytes) break;
            InboundMessage message;
            message.fd = fd;
            message.type = type;
            message.size = payload_length;
            std::memcpy(message.data, client.recv_buffer + client.parse_offset + 5, message.size);
            if (!inbound_queue_.try_push(message)) return; //queue currently full, try later NOTE: could spin instead

            //message was sent
            client.parse_offset += 5 + payload_length;
        }


        if (client.parse_offset > 0) {
            size_t remaining = client.recv_bytes - client.parse_offset;
            if (remaining > 0) {
                std::memmove(client.recv_buffer, client.recv_buffer + client.parse_offset, remaining);
            }
            client.recv_bytes = remaining;
            client.parse_offset = 0;
        }
    }
}

void ExchangeServer::drain_outbound_queue(){
    OutboundMessage message;
    while(auto result = outbound_queue_.try_pop()){
        OutboundMessage message = std::move(*result);
        if(!clients_[message.fd].active) continue;
        ClientState& client = clients_[message.fd];
        client.write_buffer.insert(client.write_buffer.end(), message.data, message.data + message.size);
        pending_writes_.push_back(message.fd);
    }

}


void ExchangeServer::try_send(int fd){
    ClientState& client = clients_[fd];
    if (!client.active) return;

    if(client.write_buffer.empty()) return; //early exit if the buffer is empty, nothing to send/write
    ssize_t n = send(fd, client.write_buffer.data() + client.sent_bytes, client.write_buffer.size() - client.sent_bytes, MSG_DONTWAIT);
    if(n > 0){
        client.sent_bytes += n;
    }else if(n == 0){
        disconnect_client(fd);
        return;
    }else if(n == -1){
        if(errno == EAGAIN || errno == EWOULDBLOCK) return;
        disconnect_client(fd);
        return;
        // throw std::runtime_error("error receiving client payload");
    }
    if(client.sent_bytes == client.write_buffer.size()){
        //success, complete message sent
        client.write_buffer.clear();
        client.sent_bytes = 0;
        return;
    }else{
        //didnt manage to send whole message in one, start listening for writes on this fd
        set_epoll_write(fd, true);
    }
}


void ExchangeServer::handle_write(int fd){
    ClientState& client = clients_[fd];
    if (!client.active) return;

    ssize_t n = send(fd, client.write_buffer.data() + client.sent_bytes, client.write_buffer.size() - client.sent_bytes, MSG_DONTWAIT);
    if(n > 0){
        client.sent_bytes += n;
    }else if(n == 0){
        disconnect_client(fd);
        return;
    }else if(n == -1){
        //try again later errors, client is still alive
        if(errno == EAGAIN || errno == EWOULDBLOCK) return;
        //if a client is having other errors, we cant stop our server, simply disconnect them
        disconnect_client(fd);
        return;
    }
    //not finished writing to the buffer yet
    if(client.sent_bytes < client.write_buffer.size()){
        return;
    }
    //reset state ready to write next message
    client.write_buffer.clear();
    client.sent_bytes = 0;
    //no longer listen for write on this fd
    set_epoll_write(fd, false);

}

void ExchangeServer::set_epoll_write(int fd, bool enable){
    epoll_event ev{};
    //if enable is true, listen for in and out, else listen for in only
    ev.events = EPOLLIN | (enable ? EPOLLOUT : 0);
    ev.data.fd = fd;
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
}



void ExchangeServer::disconnect_client(int fd){
    //deregister the fd from epoll, no longer receive events about this fd
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    //remove the client from our mapping of client fd to ClientState
    clients_[fd] = ClientState{};//reset to new ClientState, instantiatied with active=false
}    


void ExchangeServer::flush_pending_writes(){
    for(int fd : pending_writes_){
        try_send(fd);
    }
    pending_writes_.clear();
}




