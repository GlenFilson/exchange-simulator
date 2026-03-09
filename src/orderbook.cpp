#include "orderbook.hpp"


#include <map>
#include <list>
#include <optional>
#include <cstdint>
#include <iterator>
#include <ostream>


//return std::optional to protect incase the maps are empty
std::optional<double> OrderBook::best_bid() const {
    if (!bids_.empty()) return bids_.begin()->first;
    return std::nullopt;

}

std::optional<double> OrderBook::best_ask() const {   
    if (!asks_.empty()) return asks_.begin()->first;
    return std::nullopt;
}

void OrderBook::add_order(const Order& order){
    if(order.orderType() == OrderType::LIMIT){
        if(order.side() == Side::ASK){
            auto& list = asks_[order.price()];
            list.push_back(order);

            order_map_[order.id()] = OrderLocation{
                .side = Side::ASK,
                .price = order.price(),
                //end returns an iterator past the last element, we use std::prev to get the last element, previous to end
                .list_position = std::prev(list.end())
            };
        }else if (order.side() == Side::BID){
            auto& list = bids_[order.price()];
            list.push_back(order);
            order_map_[order.id()] = OrderLocation{
                .side = Side::BID,
                .price = order.price(),
                .list_position = std::prev(list.end())
            };
        }
    }else if (order.orderType() == OrderType::MARKET){
        //its a market order, we wont add it to book, will pass to matching engine
        
    }

}

void OrderBook::cancel_order(uint64_t id){
    //.find() returns the element at the position if it exists, else it returns an interator for the end of the map
    //get it outside of the conditional to avoid duplication, alternative is to find if it exists in condition, then in the body find it again to do operation
    auto it = order_map_.find(id);
    if(it != order_map_.end()){
        //in this scope we know we do not have the iterator to the end, so is std::pair <key, value>
        //second is the value at that key
        OrderLocation orderLocation = it -> second;
        if(orderLocation.side == Side::ASK){
            auto& list = asks_[orderLocation.price];
            list.erase(orderLocation.list_position);
            //we erase the level if it is empty to maintain consistency elsewhere
            //cleaning this up means that we can always gaurantee an order exists on a given price level
            //because of this, we can say best bid and ask are always max and min respectively
            if (list.empty()) asks_.erase(orderLocation.price); 
        }else if(orderLocation.side == Side::BID){
            auto& list = bids_[orderLocation.price];
            list.erase(orderLocation.list_position);
            if (list.empty()) bids_.erase(orderLocation.price);
        }
        //finally, remove the order from the order_map_ once it has been removed from the levels
        order_map_.erase(it);


    }else{
        throw std::invalid_argument("order id not found in the book");
    }

}

void OrderBook::print_depth(std::ostream& out, int levels) const{
        out << "---order book---" << "\n";
        std::vector<std::pair<double, uint32_t>> asks;
        asks.reserve(std::min(levels, static_cast<int>(asks_.size())));
        auto asks_it = asks_.begin();
        for(int i = 0; i < levels && asks_it!=asks_.end(); i ++){
            double price = asks_it->first;
            uint32_t total_quantity{0};
            for (const Order& order : asks_it->second){
                total_quantity+=order.quantity();
            }
            asks.emplace_back(price, total_quantity);
            //advance the iterator
            ++asks_it;
        }
        //print the asks
        for (auto rit = asks.rbegin(); rit!=asks.rend(); ++rit){
            out << " ASK " << rit->first << " | " << rit->second << "\n";
        }
        out << "--------------" << "\n";
        auto bids_it = bids_.begin();
        for(int i = 0; i < levels && bids_it!= bids_.end(); i++){
            double price = bids_it->first;
            uint32_t total_quantity{0};
            for (const Order& order : bids_it->second){
                total_quantity+=order.quantity();
            }
            out << " BID " << price << " | " << total_quantity << "\n";
            ++bids_it;
        }
        

}



