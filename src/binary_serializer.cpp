
#include "binary_serializer.hpp"
#include <cstring>


Message BinarySerializer::serialize_order(const Order& order){
        constexpr size_t PAYLOAD_SIZE = sizeof(uint64_t) + sizeof(double) 
                              + sizeof(uint32_t) + sizeof(Side) + sizeof(OrderType);
        std::vector<uint8_t> payload(PAYLOAD_SIZE);
        //memcpy(destination address, source address, size: number of bytes to copy)
        /*Order
        uint64_t timestamp_;
        uint64_t id_;
        double price_;
        uint32_t quantity_;
        Side side_;
        OrderType orderType_;
        */
        //.data() is a pointer to the data part of the vector
        size_t offset = 0;
        //timestamp - dont actually need timestamp, exchange will just create one on receipt
        // dont care when it was sent from the client, the exchange cares when it was received
        // std::memcpy(payload.data() + offset, &order.timestamp(), sizeof(uint64_t));
        // offset+=sizeof(uint64_t);
        //id
        uint64_t id = order.id();
        std::memcpy(payload.data() + offset, &id, sizeof(uint64_t));
        offset+=sizeof(uint64_t);
        //price
        double price = order.price();
        std::memcpy(payload.data() + offset, &price, sizeof(double));
        offset+=sizeof(double);
        //quantity
        uint32_t quantity = order.quantity();
        std::memcpy(payload.data() + offset, &quantity, sizeof(uint32_t));
        offset+=sizeof(uint32_t);
        //side
        Side side = order.side();
        std::memcpy(payload.data() + offset, &side, sizeof(Side));
        offset+=sizeof(Side);
        //orderType
        OrderType orderType = order.orderType();
        std::memcpy(payload.data() + offset, &orderType, sizeof(OrderType));
        

       Message message{
            .type = MessageType::NEW_ORDER,
            .payload = payload
        };
        return message;
    }
 
Order BinarySerializer::deserialize_order(const Message& message){
        const std::vector<uint8_t>& payload = message.payload;
        size_t offset = 0;
        //id
        uint64_t id;
        std::memcpy(&id, payload.data() + offset, sizeof(uint64_t));
        offset+=sizeof(uint64_t);
        double price;
        std::memcpy(&price, payload.data() + offset, sizeof(double));
        offset+=sizeof(double);
        //quantity
        uint32_t quantity;
        std::memcpy(&quantity, payload.data() + offset, sizeof(quantity));
        offset+=sizeof(quantity);
        //side
        Side side;
        std::memcpy(&side, payload.data() + offset, sizeof(Side));
        offset+=sizeof(Side);
        //orderType
        OrderType orderType;
        std::memcpy(&orderType, payload.data() + offset, sizeof(OrderType));


        Order order(id, price, quantity, side, orderType);
        return order;
    }

    Message BinarySerializer::serialize_cancel(uint64_t id){
        std::vector<uint8_t> payload(sizeof(uint64_t));
        std::memcpy(payload.data(), &id, sizeof(uint64_t));

        Message message{
            .type = MessageType::CANCEL_ORDER,
            .payload = payload
        };
        return message;
    }

    uint64_t BinarySerializer::deserialize_cancel(const Message& message){
        uint64_t id;
        std::memcpy(&id, message.payload.data(), sizeof(uint64_t));
        return id;
    }

    Message BinarySerializer::serialize_acknowledgement(const Acknowledgement& acknowledgement){
        std::vector<uint8_t> payload(sizeof(uint64_t));
        std::memcpy(payload.data(), &acknowledgement.id, sizeof(uint64_t));
        
        Message message{
            .type = MessageType::ORDER_ACK,
            .payload = payload
        };
        return message;
    }

    Acknowledgement BinarySerializer::deserialize_acknowledgement(const Message& message){
        uint64_t id;
        std::memcpy(&id, message.payload.data(), sizeof(uint64_t));
        Acknowledgement acknowledgement{
            .id = id
        };
        return acknowledgement;
    }

    Message BinarySerializer::serialize_rejection(const Rejection& rejection){
        size_t reason_length = rejection.reason.size();
        //id: 8bytes, size of string: 4bytes, the string itself: size defined by size of string previous field
        std::vector<uint8_t> payload(sizeof(uint64_t) + sizeof(uint32_t) + reason_length);
        size_t offset = 0;
        uint64_t id = rejection.id;
        std::memcpy(payload.data() + offset, &id, sizeof(uint64_t));
        offset+=sizeof(uint64_t);

        std::memcpy(payload.data() + offset, &reason_length, sizeof(uint32_t));
        offset+=sizeof(uint32_t);

        //copy into the data segment of the string object, cant do just &reason as that points to start of string object, not necessarily the data segment
        std::memcpy(payload.data() + offset, rejection.reason.data(), reason_length);

        Message message{
            .type = MessageType::REJECT,
            .payload = payload
        };
        return message;
    }

    Rejection BinarySerializer::deserialize_rejection(const Message& message){
        size_t offset = 0;
        
        uint64_t id;
        std::memcpy(&id, message.payload.data() + offset, sizeof(uint64_t));
        offset+=sizeof(uint64_t);

        uint32_t reason_length;
        std::memcpy(&reason_length, message.payload.data() + offset, sizeof(uint32_t));
        offset+=sizeof(uint32_t);

        std::string reason(reinterpret_cast<const char*>(message.payload.data() + offset), reason_length);

        Rejection rejection {
            .id = id,
            .reason = reason
        };
        return rejection;
    }

Message BinarySerializer::serialize_trade(const Trade& trade){
    constexpr size_t PAYLOAD_SIZE = sizeof(double) + sizeof(uint64_t) + sizeof(uint64_t)
                            + sizeof(uint32_t) + sizeof(uint64_t);
    std::vector<uint8_t> payload(PAYLOAD_SIZE);
    size_t offset = 0;

    std::memcpy(payload.data() + offset, &trade.price, sizeof(double));
    offset+=sizeof(double);

    std::memcpy(payload.data() + offset, &trade.buyer_order_id, sizeof(uint64_t));
    offset+=sizeof(uint64_t);

    std::memcpy(payload.data() + offset, &trade.seller_order_id, sizeof(uint64_t));
    offset+=sizeof(uint64_t);

    std::memcpy(payload.data() + offset, &trade.quantity, sizeof(uint32_t));
    offset+=sizeof(uint32_t);
    
    std::memcpy(payload.data() + offset, &trade.timestamp, sizeof(uint64_t));

    Message message{
        .type = MessageType::TRADE,
        .payload = payload
    };
    return message;


}  

Trade BinarySerializer::deserialize_trade(const Message& message){
    size_t offset = 0;
    
    double price;
    std::memcpy(&price, message.payload.data() + offset, sizeof(double));
    offset+=sizeof(double);

	uint64_t buyer_order_id;
    std::memcpy(&buyer_order_id, message.payload.data() + offset, sizeof(uint64_t));
    offset+=sizeof(uint64_t);

	uint64_t seller_order_id;
    std::memcpy(&seller_order_id, message.payload.data() + offset, sizeof(uint64_t));
    offset+=sizeof(uint64_t);

	uint32_t quantity;
    std::memcpy(&quantity, message.payload.data() + offset, sizeof(uint32_t));
    offset+=sizeof(uint32_t);

	uint64_t timestamp;
    std::memcpy(&timestamp, message.payload.data() + offset, sizeof(uint64_t));

    Trade trade{
        .price = price,
        .buyer_order_id = buyer_order_id,
        .seller_order_id = seller_order_id,
        .quantity = quantity,
        .timestamp = timestamp
    };
    return trade;
}