#include <iostream>
#include "order.hpp"
#include "order_simulator.hpp"
#include "orderbook.hpp"


int main(){
    OrderBook orderBook;
    OrderSimulator orderSimulator(500.0);
    std::vector<Order> simulated_orders = orderSimulator.generate_n_orders(5);
    for (Order& order : simulated_orders){
          orderBook.add_order(order);
        //   order.toString();
          auto bid = orderBook.best_bid();
          auto ask = orderBook.best_ask();
          std::cout 
          << "best bid: " 
          << (bid.has_value() ? std::to_string(bid.value()) : "none") 
          << " | best ask: " 
          << (ask.has_value() ? std::to_string(ask.value()) : "none") 
          << std::endl;
    }
    // Order bid1(1, 100.0, 10, Side::BID, OrderType::LIMIT);
    // Order bid2(2, 105.0, 10, Side::BID, OrderType::LIMIT);
    // Order ask1(3, 110.0, 10, Side::ASK, OrderType::LIMIT);

    // orderBook.add_order(bid1);
    // orderBook.add_order(bid2);
    // orderBook.add_order(ask1);

    // std::cout << "best bid: " << orderBook.best_bid().value() << "\n";   // expect 105
    // std::cout << "best ask: " << orderBook.best_ask().value() << "\n";   // expect 110

    // orderBook.cancel_order(2);  // cancel the 105 bid

    // std::cout << "after cancel:\n";
    // std::cout << "best bid: " << orderBook.best_bid().value() << "\n";   // expect 100
    // std::cout << "best ask: " << orderBook.best_ask().value() << "\n";   // expect 110

    return 0;
}