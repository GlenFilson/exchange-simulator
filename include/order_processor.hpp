#pragma once

#include "inbound_message.hpp"
#include "outbound_message.hpp"
#include "thread_safe_queue.hpp"
#include "matching_engine.hpp"
#include "orderbook.hpp"
#include "serializer.hpp"
#include <cstdint>
#include "spsc_ring_buffer.hpp"
#include "types.hpp"

class OrderProcessor{
    public:
        OrderProcessor(SPSCRingBuffer<InboundMessage, DEFAULT_RING_CAPACITY>& iq, SPSCRingBuffer<OutboundMessage, DEFAULT_RING_CAPACITY>& oq, MatchingEngine& me, OrderBook& ob, Serializer& s)
            : inbound_queue_{iq}
            , outbound_queue_{oq}
            , matching_engine_{me}
            , orderbook_{ob}
            , serializer_{s}
        {}
        void run();
        void process(InboundMessage& message);


    private:
        // ThreadSafeQueue<InboundMessage>& inbound_queue_;
        // ThreadSafeQueue<OutboundMessage>& outbound_queue_;
        SPSCRingBuffer<InboundMessage, DEFAULT_RING_CAPACITY>& inbound_queue_;
        SPSCRingBuffer<OutboundMessage, DEFAULT_RING_CAPACITY>& outbound_queue_;
        MatchingEngine& matching_engine_;
        OrderBook& orderbook_;
        std::vector<Trade> trades_;
        Serializer& serializer_;
        std::unordered_map<OrderId, int> order_to_client_fd_;

};
