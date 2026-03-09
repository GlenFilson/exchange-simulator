#include <iostream>
#include "order.hpp"
#include "order_simulator.hpp"
#include "orderbook.hpp"
#include "matching_engine.hpp"
#include <fstream>
#include <thread>
#include <chrono>

int main(){
    OrderBook orderBook;
    OrderSimulator orderSimulator(1000);
    MatchingEngine matchingEngine(orderBook);
    std::ofstream trade_log("trades.log");
    while(true){
        std::cout << "\033[2J\033[H";  // clear screen and move cursor to top
        orderBook.print_depth(std::cout, 5);
        Order order = orderSimulator.generate_order();
        std::vector<Trade> trades = matchingEngine.match(order);
        for (const Trade& trade : trades){
            trade_log << "timestamp: " << trade.timestamp << ", price: " << trade.price << ", quantity: " << trade.quantity << ", buyer_order_id: " << trade.buyer_order_id << ", seller_order_id: " << trade.seller_order_id << std::endl;

        } 
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

    }

    return 0;
}