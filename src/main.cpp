#include <iostream>
#include "order.hpp"
#include "order_simulator.hpp"
#include "orderbook.hpp"
#include "matching_engine.hpp"
#include "exchange_server.hpp"
#include "binary_serializer.hpp"
#include <fstream>
#include <thread>
#include <chrono>
#include "thread_safe_queue.hpp"
#include "inbound_message.hpp"
#include "outbound_message.hpp"
#include "order_processor.hpp"
int main(){
   

    // OrderBook order_book;
    // MatchingEngine matching_engine(order_book);
    // BinarySerializer serializer;
    // ExchangeServer server(8080, order_book, matching_engine, serializer);
    // server.start();
    // server.run();

    ThreadSafeQueue<InboundMessage> inbound_queue;
    ThreadSafeQueue<OutboundMessage> outbound_queue;

    OrderBook order_book;
    MatchingEngine matching_engine(order_book);
    BinarySerializer serializer;

    OrderProcessor processor(inbound_queue, outbound_queue, matching_engine, order_book, serializer);
    ExchangeServer server(8080, inbound_queue, outbound_queue);

    server.start();

    std::thread processor_thread(&OrderProcessor::run, &processor);

    server.run();
    
    processor_thread.join();
    return 0;
}