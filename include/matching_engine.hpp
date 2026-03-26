#pragma once

#include <vector>

#include "orderbook.hpp"
#include "trade.hpp"
#include "order.hpp"


class MatchingEngine{

    public:
    MatchingEngine(OrderBook& orderBook);
    void match(const Order& order, std::vector<Trade>& trades);
        
    private:
    OrderBook& orderBook_;


};