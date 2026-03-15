#pragma once

#include "inbound_message.hpp"
#include "outbound_message.hpp"
#include "thread_safe_queue.hpp"
#include "matching_engine.hpp"
#include "orderbook.hpp"
#include "serializer.hpp"
#include <cstdint>

class OrderProcessor{
    public:
        OrderProcessor(ThreadSafeQueue<InboundMessage>& iq, ThreadSafeQueue<OutboundMessage>& oq, MatchingEngine& me, OrderBook& ob, Serializer& s)
            : inbound_queue_{iq}
            , outbound_queue_{oq}
            , matching_engine_{me}
            , orderbook_{ob}
            , serializer_{s}
        {}
        void run();
        void process(InboundMessage& message);


    private:
        ThreadSafeQueue<InboundMessage>& inbound_queue_;
        ThreadSafeQueue<OutboundMessage>& outbound_queue_;
        MatchingEngine& matching_engine_;
        OrderBook& orderbook_;
        Serializer& serializer_;
        std::unordered_map<uint64_t, int> order_to_client_fd_;

};
