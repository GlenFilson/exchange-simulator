#pragma once

#include <vector>

#include "orderbook.hpp"
#include "trade.hpp"
#include "order.hpp"


class MatchingEngine{

    public:
    MatchingEngine(OrderBook& orderBook);
    std::vector<Trade> match(const Order& order);
        
    private:
    OrderBook& orderBook_;


};