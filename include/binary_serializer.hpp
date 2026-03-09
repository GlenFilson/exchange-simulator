#pragma once

#include "serializer.hpp"
#include "message.hpp"
#include "trade.hpp"
#include "order.hpp"
#include "acknowledgement.hpp"
#include "rejection.hpp"

class BinarySerializer : public Serializer {

    public:
        Message serialize_order(const Order& order) override;
        Order deserialize_order(const Message& message) override;

        Message serialize_cancel(uint64_t id) override;
        uint64_t deserialize_cancel(const Message& message)override;

        Message serialize_acknowledgement(const Acknowledgement& acknowledgement) override;
        Acknowledgement deserialize_acknowledgement(const Message& message) override;

        Message serialize_rejection(const Rejection& rejection) override;
        Rejection deserialize_rejection(const Message& message) override;

        Message serialize_trade(const Trade& trade) override;
        Trade deserialize_trade(const Message& message) override;


};

