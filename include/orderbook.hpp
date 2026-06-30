#pragma once
#include <map>
#include <vector>
#include <list>
#include <optional>
#include "order.hpp"
#include <cstdint>
#include <unordered_map>
#include <ostream>


class OrderBook {
    friend class MatchingEngine;

    public:
        std::optional<double> best_bid() const;
        std::optional<double> best_ask() const;
        //take reference to order, not order itself. dont copy
        void add_order(const Order& order);
        void cancel_order(uint64_t id);
        void print_depth(std::ostream& out, int levels) const;


    private:

        //maps store <price, list> where list contains orders
        //prices sorted high to low, force with comparator std::greater
        std::map<double, std::list<Order>, std::greater<double>> bids_;
        //default comparator sorts low to high
        std::map <double, std::list<Order>> asks_;
        
   
        /*     
        OrderLocation lets us access an order is O(1) average time to remove it from the book
        first query by side (bid or ask) then price as key of either bids or asks
        finally, the iterator of the list at that price level
        
        NOTE: we could just store the iterator but then we would have to dereference the iterator to get info like price and side
        uses more memory this way but we are choosing to store this metadata for ease of use and potential speed up when accessing side and price
        */
        struct OrderLocation {
            Side side;
            double price;
            std::list<Order>::iterator list_position;
        };
        //maps order ids to their OrderLocation
        std::unordered_map <uint64_t, OrderLocation> order_map_;


};