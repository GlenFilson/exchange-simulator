#include <iostream>
#include "order.hpp"
#include "order_simulator.hpp"
#include "orderbook.hpp"
#include "matching_engine.hpp"
void print_book(const OrderBook& book) {
    auto bid = book.best_bid();
    auto ask = book.best_ask();
    std::cout << "best bid: " << (bid.has_value() ? std::to_string(bid.value()) : "none")
              << " | best ask: " << (ask.has_value() ? std::to_string(ask.value()) : "none")
              << "\n";
}

int main(){
    OrderBook orderBook;
    MatchingEngine matchingEngine(orderBook);

    Order ask1(1, 100.0, 50, Side::ASK, OrderType::LIMIT);
    Order bid1(2, 100, 30, Side::BID, OrderType::LIMIT);
    Order bid2(3, 100, 80, Side::BID, OrderType::LIMIT);
    std::vector<Trade> trades;
    orderBook.add_order(ask1);
    print_book(orderBook);
    trades = matchingEngine.match(bid1);
    for (Trade trade : trades){
        std::cout << "Trade!" << "\n";
        std::cout << "price: " << trade.price << " buyer_id: " << trade.buyer_order_id << " seller_id: " << trade.seller_order_id << " quantity: " << trade.quantity << " timestamp: " << trade.timestamp << "\n";
    }
   print_book(orderBook);
    trades = matchingEngine.match(bid2);
    for (Trade trade : trades){
        std::cout << "Trade!" << "\n";
        std::cout << "price: " << trade.price << " buyer_id: " << trade.buyer_order_id << " seller_id: " << trade.seller_order_id << " quantity: " << trade.quantity << " timestamp: " << trade.timestamp << "\n";
    }
    print_book(orderBook);

    return 0;
}