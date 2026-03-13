#pragma once

#include <cstdint>
#include "message.hpp"
#include "trade.hpp"
#include "order.hpp"
#include "acknowledgement.hpp"
#include "rejection.hpp"


class Serializer {

    //NOTE: updated serialize functions to take in buffer references, instead of returning messages
    //this reduces allocations as we write directly into buffers instead of copying data and passing objects
    public:
        virtual void serialize_order(const Order& order, std::vector<uint8_t>& buffer) = 0;
        virtual Order deserialize_order(const Message& message) = 0;

        virtual void serialize_cancel(uint64_t id, std::vector<uint8_t>& buffer) = 0; 
        //cancel is just the order id to be cancelled
        virtual uint64_t deserialize_cancel(const Message& message) = 0;

        virtual void serialize_acknowledgement(const Acknowledgement& acknowledgement, std::vector<uint8_t>& buffer) = 0;
        virtual Acknowledgement deserialize_acknowledgement(const Message& message) = 0;
        
        virtual void serialize_rejection(const Rejection& rejection, std::vector<uint8_t>& buffer) = 0;
        virtual Rejection deserialize_rejection(const Message& message) = 0;
        
        virtual void serialize_trade(const Trade& trade, std::vector<uint8_t>& buffer) = 0;  
        virtual Trade deserialize_trade(const Message& message) = 0;
       
        virtual void serialize_cancel_ack(const Acknowledgement& acknowledgement, std::vector<uint8_t>& buffer) = 0;
        virtual Acknowledgement deserialize_cancel_ack(const Message& message) = 0;


        /*
        as we will have derived classes, we need to also define a virtual destructor
        without this, when a derived class is to be destructored, the base class destructor is called
        leaving potential for memory leaks
        */
        virtual ~Serializer() = default;

};