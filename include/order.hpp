//ensures the header is only included once
#pragma once
//for the fixed size integers types
#include <cstdint>
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <cstdint>

/*
using enum class instead of plain enum
plain enum can be dangerous as other enums with same name may collide
using enum class we access the enums using class:enum, so Side::BID
which is safer 
*/
enum class Side : uint8_t {
    BID,
    ASK
};

enum class OrderType : uint8_t{
    LIMIT,
    MARKET
};

class Order {
    public:
        Order(uint64_t id, int64_t price, uint32_t quantity, Side side, OrderType orderType)
        : timestamp_{static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
		std::chrono::high_resolution_clock::now().time_since_epoch()
	        ).count())}
        , id_{id}
        ,price_{price}
        ,quantity_{quantity}
        ,side_{side}
        ,orderType_{orderType}
        {
            //Validation
            if(orderType_ == OrderType::LIMIT && price <= 0) throw std::invalid_argument("limit orders cant have non-positive price");
            if(orderType_ == OrderType::MARKET && price > 0) throw std::invalid_argument("market orders cant have positive price");
            //quantity is uint so can never be negative, it is invalid when its 0
            if(quantity == 0) throw std::invalid_argument("quantity must be greater than 0");
        }

        uint64_t timestamp() const { return timestamp_;}
        uint64_t id() const {return id_;}
        int64_t price()const {return price_;}
        uint32_t quantity() const {return quantity_;}
        Side side() const {return side_;}
        OrderType orderType() const {return orderType_;}

        void reduce_quantity(uint32_t quantity){
            if (quantity > quantity_) throw std::invalid_argument("cannot reduce by more than current quantity");
            else quantity_ -= quantity;
        }

        void toString() const {
            std::cout << "timestamp: " << timestamp_ << std::endl;
            std::cout << "id: " << id_ << std::endl;
            std::cout << "price: " << price_ << std::endl; 
            std::cout << "quantity: " << quantity_ << std::endl;
            std::cout << "side: " << (side_==Side::BID ? "bid": "ask") << std::endl;
            std::cout << "orderType: " << (orderType_==OrderType::LIMIT ? "limit": "market") << std::endl << std::endl;  
        }


    private:
        uint64_t timestamp_;
        uint64_t id_;
        int64_t price_;
        uint32_t quantity_;
        Side side_;
        OrderType orderType_;
};