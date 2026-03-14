
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
ExchangeServer::ExchangeServer(uint16_t port, OrderBook& order_book, MatchingEngine& matching_engine, Serializer& serializer)
    : port_{port}
    , order_book_{order_book}
    , matching_engine_{matching_engine}
    ,serializer_{serializer}
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
        flush_pending_writes();

        //n: number of events occured + written, events: where to write about any fds ready for reading, 64: number of events we can write to, -1: wait infinitely
        int n = epoll_wait(epoll_fd_, events, 64, -1);
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

    if(client.read_phase == ReadPhase::HEADER){
        //read into header_buffer offset by the number of bytes we have already read
        //we want to read another 5-bytes_read, basically read until we have read 5 bytes, filled the buffer
        ssize_t n = recv(fd, client.header_buffer + client.bytes_read, 5-client.bytes_read, MSG_DONTWAIT);//non-blocking recv
        if(n > 0){
            client.bytes_read += n;
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
        //if we have not yet filled the buffer, return
        if(client.bytes_read < 5){
            return;
        }
        //if we have exactly filled the buffer (5 bytes), move on to payload reading phase
        //first byte in buffer is the MessageType
        client.message_type = static_cast<MessageType>(client.header_buffer[0]);
        //the payload length is the next 4 bytes, (1-5)
        std::memcpy(&client.payload_length, client.header_buffer+1, sizeof(uint32_t));
        //convert from network to host byte order
        client.payload_length = ntohl(client.payload_length);
        //now we know how big the payload is, resize the buffer ready to accept it
        client.read_buffer.resize(client.payload_length);
        client.bytes_read = 0;//reset bytes_read for reading payload
        //we are set up to enter payload reading phase, switch read_phase
        if(client.payload_length != 0){
            client.read_phase = ReadPhase::PAYLOAD;
        }else{
            //handle the case where the payload length is 0, a client is sending keep-alive pings
            //we dont want to disconnect them, so just reset their state and ignore
            client.read_phase = ReadPhase::HEADER;
        }
        
    }
    
    if(client.read_phase == ReadPhase::PAYLOAD){
        //write data into the data section of the read_buffer vector
        ssize_t n = recv(fd, client.read_buffer.data() + client.bytes_read, client.payload_length-client.bytes_read, MSG_DONTWAIT);
        if(n > 0){
            client.bytes_read += n;
        }else if(n == 0){//connection closed
            disconnect_client(fd);
            return;
        }else if(n == -1){
            //acceptable errors, just exit
            if(errno == EAGAIN || errno == EWOULDBLOCK) return;
            disconnect_client(fd);
            return;
            // throw std::runtime_error("error receiving client payload");
        }
        //if we have not yet filled the buffer, return
        if(client.bytes_read < client.payload_length){
            return;
        }
        //if we have read the entire payload, construct and handle the message
        Message message{
            .type = client.message_type,
            .payload = client.read_buffer//NOTE: currently deep copying, look into improving this
        };
        handle_message(fd, message);
        //now we are done with this message, reset clients state ready to read a new message
        client.read_phase = ReadPhase::HEADER;
        client.bytes_read = 0;
    }
}
void ExchangeServer::try_send(int fd){
    ClientState& client = clients_[fd];
    if (!client.active) return;

    if(client.write_buffer.empty()) return; //early exit if the buffer is empty, nothing to send/write
    ssize_t n = send(fd, client.write_buffer.data() + client.bytes_sent, client.write_buffer.size() - client.bytes_sent, MSG_DONTWAIT);
    if(n > 0){
        client.bytes_sent += n;
    }else if(n == 0){
        disconnect_client(fd);
        return;
    }else if(n == -1){
        if(errno == EAGAIN || errno == EWOULDBLOCK) return;
        disconnect_client(fd);
        return;
        // throw std::runtime_error("error receiving client payload");
    }
    if(client.bytes_sent == client.write_buffer.size()){
        //success, complete message sent
        client.write_buffer.clear();
        client.bytes_sent = 0;
        return;
    }else{
        //didnt manage to send whole message in one, start listening for writes on this fd
        set_epoll_write(fd, true);
    }
}


void ExchangeServer::handle_write(int fd){
    ClientState& client = clients_[fd];
    if (!client.active) return;

    ssize_t n = send(fd, client.write_buffer.data() + client.bytes_sent, client.write_buffer.size() - client.bytes_sent, MSG_DONTWAIT);
    if(n > 0){
        client.bytes_sent += n;
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
    if(client.bytes_sent < client.write_buffer.size()){
        return;
    }
    //reset state ready to write next message
    client.write_buffer.clear();
    client.bytes_sent = 0;
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

void ExchangeServer::handle_message(int client_fd, const Message& message){
/*
enum class MessageType : uint8_t {
    NEW_ORDER = 1,
    CANCEL_ORDER = 2,
    ORDER_ACK = 3,
    REJECT = 4,
    TRADE = 5
};
*/  
    ClientState& client = clients_[client_fd];



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
                //add the order id to client file descriptor mapping
                order_to_client_fd_[order.id()] = client_fd;
                //process any trades made, and send trade messages
                for (const Trade& trade : trades){
                    //notifies the taker that a trade was made
                    serializer_.serialize_trade(trade, client.write_buffer);
                    
 
                    //need to also notify the maker, the counterparty of the trade
                    int counterparty_fd = order_to_client_fd_[order.side() == Side::ASK ? trade.buyer_order_id : trade.seller_order_id];
                    serializer_.serialize_trade(trade, clients_[counterparty_fd].write_buffer);
                    pending_writes_.push_back(counterparty_fd);

                }
                //only ack order after any trades have been sent
                Acknowledgement ack{
                    .id = order.id()
                };
                serializer_.serialize_acknowledgement(ack, client.write_buffer);
            }catch(const std::exception& e){
                Rejection rejection{
                    .id = raw_id,
                    .reason = std::string(e.what())
                };
                serializer_.serialize_rejection(rejection, client.write_buffer);
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
                serializer_.serialize_cancel_ack(cancel_ack, client.write_buffer);
                order_book_.cancel_order(id);
                order_to_client_fd_.erase(id);
            }catch(const std::exception& e){
                Rejection rejection{
                    .id = raw_id,
                    .reason = std::string(e.what())
                };
                serializer_.serialize_rejection(rejection, client.write_buffer);
                break;
            }
            break;
        }
        default:
            throw std::runtime_error("unknown message type");
            break;
    }

    pending_writes_.push_back(client_fd);
}

void ExchangeServer::flush_pending_writes(){
    for(int fd : pending_writes_){
        try_send(fd);
    }
    pending_writes_.clear();
}


