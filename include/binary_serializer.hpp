#pragma once

#include "serializer.hpp"
#include "message.hpp"
#include "trade.hpp"
#include "order.hpp"
#include "acknowledgement.hpp"
#include "rejection.hpp"

class BinarySerializer : public Serializer {

    public:
        void serialize_order(const Order& order, std::vector<uint8_t>& buffer) override;
        Order deserialize_order(const Message& message) override;

        void serialize_cancel(uint64_t id, std::vector<uint8_t>& buffer) override;
        uint64_t deserialize_cancel(const Message& message)override;

        void serialize_acknowledgement(const Acknowledgement& acknowledgement, std::vector<uint8_t>& buffer) override;
        Acknowledgement deserialize_acknowledgement(const Message& message) override;

        void serialize_rejection(const Rejection& rejection, std::vector<uint8_t>& buffer) override;
        Rejection deserialize_rejection(const Message& message) override;

        void serialize_trade(const Trade& trade, std::vector<uint8_t>& buffer) override;
        Trade deserialize_trade(const Message& message) override;

        void serialize_cancel_ack(const Acknowledgement& acknowledgement, std::vector<uint8_t>& buffer) override;
        Acknowledgement deserialize_cancel_ack(const Message& message) override;


};

