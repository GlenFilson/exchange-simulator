

#include <vector>
#include "matching_engine.hpp"
#include "trade.hpp"
#include <chrono>


MatchingEngine::MatchingEngine(OrderBook& orderBook)
    : orderBook_{orderBook}
    {}

void MatchingEngine::match(const Order& order, std::vector<Trade>& trades){
    uint32_t remaining_quantity = order.quantity();
    trades.clear();
    if(order.side() == Side::BID){
        while(remaining_quantity > 0 && !(orderBook_.asks_.empty())){
            //access the first element in asks, the best price, lowest
            double best_ask = orderBook_.asks_.begin()->first;
            //if its a limit order and the price dosent cross the book, cant match, break
            if(order.orderType() == OrderType::LIMIT && order.price() < best_ask){
                break;   
            }
            //get reference to the list at the best ask
            auto& orders = orderBook_.asks_[best_ask];
            //match until either the order is filled of the level is wiped out
            while (remaining_quantity > 0 && !orders.empty()){
                //get the order at the front of the list, FIFO, time priority
                Order& matched_order = orders.front();
                uint32_t filled_quantity = std::min(matched_order.quantity(), remaining_quantity);

                Trade trade{
                    .price = best_ask,
                    .buyer_order_id = order.id(),
                    .seller_order_id = matched_order.id(),
                    .quantity = filled_quantity,
                    .timestamp = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                           std::chrono::high_resolution_clock::now().time_since_epoch())
                                                           .count())
                };
                trades.push_back(trade);
                remaining_quantity -= filled_quantity;
                
                //after successful trade, check if matched_order was fully filled
                if (matched_order.quantity() == filled_quantity){
                    //quantity was fully filled, remove the order from the level
                    orderBook_.order_map_.erase(matched_order.id());
                    orders.pop_front();
                }else{
                    //order still has some quantity left, update its quantity
                    matched_order.reduce_quantity(filled_quantity);
                }   

                }
            //check if the level is empty, if so remove it from the asks
            if (orders.empty()) orderBook_.asks_.erase(best_ask);
            }
        if (remaining_quantity > 0 && order.orderType() == OrderType::LIMIT){
            //original order is const so cant modify its quantity, need to create new one
            //need to add the remaining order with the updated remaining quantity
            Order remaining_order(order.id(), order.price(), remaining_quantity, order.side(), order.orderType());
            orderBook_.add_order(remaining_order);
        }
    }else if(order.side() == Side::ASK){
        while(remaining_quantity > 0 && !(orderBook_.bids_.empty())){
            double best_bid = orderBook_.bids_.begin()->first;
            //order dosent cross book, cant match. asking more than best bid
            if (order.orderType() == OrderType::LIMIT && order.price() > best_bid){
                break;
            }
            auto& orders = orderBook_.bids_[best_bid];
            while(remaining_quantity > 0 && !orders.empty()){
                Order& matched_order = orders.front();
                uint32_t filled_quantity = std::min(matched_order.quantity(), remaining_quantity);

                Trade trade {
                    .price = best_bid,
                    .buyer_order_id = matched_order.id(),
                    .seller_order_id = order.id(),
                    .quantity = filled_quantity,
                    .timestamp = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                           std::chrono::high_resolution_clock::now().time_since_epoch())
                                                           .count())
                };
                trades.push_back(trade);
                remaining_quantity -= filled_quantity;

                if(matched_order.quantity() == filled_quantity){
                    orderBook_.order_map_.erase(matched_order.id());
                    orders.pop_front();
                }else{
                    matched_order.reduce_quantity(filled_quantity);
                }
                }
            if(orders.empty()) orderBook_.bids_.erase(best_bid);
            }
        if (remaining_quantity > 0 && order.orderType() == OrderType::LIMIT){
            Order remaining_order(order.id(), order.price(), remaining_quantity, order.side(), order.orderType());
            orderBook_.add_order(remaining_order);
        }
    }
}