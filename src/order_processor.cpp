#include "order_processor.hpp"
#include <cstring>
void OrderProcessor::run()
{

    while (true)
    {
        std::optional<InboundMessage> result = inbound_queue_.try_pop();
        if (result.has_value())
        {
            InboundMessage message = std::move(*result);
            process(message);
        }else {
            asm volatile("pause");
        }
    }
}

void OrderProcessor::process(InboundMessage &i_message)
{
    deserialization_message_.type = i_message.type;
    deserialization_message_.payload.assign(i_message.data, i_message.data + i_message.size);

    switch (i_message.type)
    {
    case MessageType::NEW_ORDER:
    {
        uint64_t raw_id;
        std::memcpy(&raw_id, i_message.data, sizeof(uint64_t));
        try
        {
            Order order = serializer_.deserialize_order(deserialization_message_);
            matching_engine_.match(order, trades_);
            order_to_client_fd_[order.id()] = i_message.fd;
            for (const Trade &trade : trades_)
            {
                //taker notification
                taker_buffer_.clear();
                serializer_.serialize_trade(trade, taker_buffer_);
                OutboundMessage taker_message;
                taker_message.fd = i_message.fd;
                taker_message.size = taker_buffer_.size();
                std::memcpy(taker_message.data, taker_buffer_.data(), taker_message.size);
                while (!outbound_queue_.try_push(taker_message)){}

                //maker notification
                maker_buffer_.clear();
                serializer_.serialize_trade(trade, maker_buffer_);
                OutboundMessage maker_message;
                maker_message.fd = order_to_client_fd_[order.side() == Side::ASK ? trade.buyer_order_id : trade.seller_order_id];
                maker_message.size = maker_buffer_.size();
                std::memcpy(maker_message.data, maker_buffer_.data(), maker_message.size);
                // send counterparty straight away, spin until its sent
                while (!outbound_queue_.try_push(maker_message)){}
            }
            taker_buffer_.clear();
            Acknowledgement ack{.id = order.id()};
            serializer_.serialize_acknowledgement(ack, taker_buffer_);
            OutboundMessage ack_message;
            ack_message.fd = i_message.fd;
            ack_message.size = taker_buffer_.size();
            std::memcpy(ack_message.data, taker_buffer_.data(), ack_message.size);
            while (!outbound_queue_.try_push(ack_message)){}
        }
        catch (const std::exception &e)
        {
            taker_buffer_.clear();
            Rejection rejection{
                .id = raw_id,
                .reason = std::string(e.what())};
            serializer_.serialize_rejection(rejection, taker_buffer_);
            OutboundMessage rejection_message;
            rejection_message.fd = i_message.fd;
            rejection_message.size = taker_buffer_.size();
            std::memcpy(rejection_message.data, taker_buffer_.data(), rejection_message.size);
            while (!outbound_queue_.try_push(rejection_message)){}
        }
        break;
    }

    case MessageType::CANCEL_ORDER:
    {

        uint64_t raw_id;
        std::memcpy(&raw_id, i_message.data, sizeof(uint64_t));
        try
        {
            uint64_t id = serializer_.deserialize_cancel(deserialization_message_);
            Acknowledgement cancel_ack{
                .id = id};
            taker_buffer_.clear();
            serializer_.serialize_cancel_ack(cancel_ack, taker_buffer_);
            orderbook_.cancel_order(id);
            order_to_client_fd_.erase(id);
            OutboundMessage ack_message;
            ack_message.fd = i_message.fd;
            ack_message.size = taker_buffer_.size();
            std::memcpy(ack_message.data, taker_buffer_.data(), ack_message.size);
            while (!outbound_queue_.try_push(ack_message)){}
        }
        catch (const std::exception &e)
        {
            taker_buffer_.clear();
            Rejection rejection{
                .id = raw_id,
                .reason = std::string(e.what())};
            serializer_.serialize_rejection(rejection, taker_buffer_);
            OutboundMessage rejection_message;
            rejection_message.fd = i_message.fd;
            rejection_message.size = taker_buffer_.size();
            std::memcpy(rejection_message.data, taker_buffer_.data(), rejection_message.size);
            while (!outbound_queue_.try_push(rejection_message)){}
        }
        break;
    }
    default:
        throw std::runtime_error("unknown message type");
        return;
    }
}