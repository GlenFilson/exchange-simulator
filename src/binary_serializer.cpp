
#include "binary_serializer.hpp"
#include <cstring>
#include <arpa/inet.h>

void BinarySerializer::serialize_order(const Order& order, std::vector<uint8_t>& buffer){
    //memcpy(destination address, source address, size: number of bytes to copy)
    /*Order
    uint64_t timestamp_;
    uint64_t id_;
    int64_t price_;
    uint32_t quantity_;
    Side side_;
    OrderType orderType_;
    */
    constexpr size_t PAYLOAD_SIZE = sizeof(uint64_t) + sizeof(int64_t)
    + sizeof(uint32_t) + sizeof(Side) + sizeof(OrderType);
    
    //offset begins at the current size of the buffer, we are appending to it
    size_t offset = buffer.size();
    
    //5 bytes for header (1 byte message type, 4 bytes payload length)
    buffer.resize(offset + 5 + PAYLOAD_SIZE);

        MessageType type = MessageType::NEW_ORDER;
        std::memcpy(buffer.data() + offset, &type, sizeof(MessageType));
        offset+=sizeof(MessageType);

        //payload length
        uint32_t net_length = htonl(static_cast<uint32_t>(PAYLOAD_SIZE));
        std::memcpy(buffer.data() + offset, &net_length, sizeof(uint32_t));
        offset+=sizeof(uint32_t);

        //timestamp - dont actually need timestamp, exchange will just create one on receipt
        // dont care when it was sent from the client, the exchange cares when it was received
        // std::memcpy(payload.data() + offset, &order.timestamp(), sizeof(uint64_t));
        // offset+=sizeof(uint64_t);
        //id
        uint64_t id = order.id();
        std::memcpy(buffer.data() + offset, &id, sizeof(uint64_t));
        offset+=sizeof(uint64_t);
        //price
        int64_t price = order.price();
        std::memcpy(buffer.data() + offset, &price, sizeof(int64_t));
        offset+=sizeof(int64_t);
        //quantity
        uint32_t quantity = order.quantity();
        std::memcpy(buffer.data() + offset, &quantity, sizeof(uint32_t));
        offset+=sizeof(uint32_t);
        //side
        Side side = order.side();
        std::memcpy(buffer.data() + offset, &side, sizeof(Side));
        offset+=sizeof(Side);
        //orderType
        OrderType orderType = order.orderType();
        std::memcpy(buffer.data() + offset, &orderType, sizeof(OrderType));
    }
 
Order BinarySerializer::deserialize_order(const Message& message){
        const std::vector<uint8_t>& payload = message.payload;
        size_t offset = 0;
        //id
        uint64_t id;
        std::memcpy(&id, payload.data() + offset, sizeof(uint64_t));
        offset+=sizeof(uint64_t);
        int64_t price;
        std::memcpy(&price, payload.data() + offset, sizeof(int64_t));
        offset+=sizeof(int64_t);
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

void BinarySerializer::serialize_cancel(uint64_t id, std::vector<uint8_t>& buffer){ 
    constexpr size_t PAYLOAD_SIZE = sizeof(uint64_t);
    size_t offset = buffer.size();
    buffer.resize(offset + 5 + PAYLOAD_SIZE);

    MessageType type = MessageType::CANCEL_ORDER;
    std::memcpy(buffer.data() + offset, &type, sizeof(MessageType));
    offset+=sizeof(MessageType);

    uint32_t net_length = htonl(static_cast<uint32_t>(PAYLOAD_SIZE));
    std::memcpy(buffer.data() + offset, &net_length, sizeof(uint32_t));
    offset+=sizeof(uint32_t);


    std::memcpy(buffer.data() + offset, &id, sizeof(uint64_t));
}

uint64_t BinarySerializer::deserialize_cancel(const Message& message){
    uint64_t id;
    std::memcpy(&id, message.payload.data(), sizeof(uint64_t));
    return id;
}

void BinarySerializer::serialize_acknowledgement(const Acknowledgement& acknowledgement, std::vector<uint8_t>& buffer){
    constexpr size_t PAYLOAD_SIZE = sizeof(uint64_t);
    size_t offset = buffer.size();
    buffer.resize(offset + 5 + PAYLOAD_SIZE);

    MessageType type = MessageType::ORDER_ACK;
    std::memcpy(buffer.data() + offset, &type, sizeof(MessageType));
    offset+=sizeof(MessageType);

    uint32_t net_length = htonl(static_cast<uint32_t>(PAYLOAD_SIZE));
    std::memcpy(buffer.data() + offset, &net_length, sizeof(uint32_t));
    offset+=sizeof(uint32_t);

    std::memcpy(buffer.data() + offset, &acknowledgement.id, sizeof(uint64_t));

}

Acknowledgement BinarySerializer::deserialize_acknowledgement(const Message& message){
    uint64_t id;
    std::memcpy(&id, message.payload.data(), sizeof(uint64_t));
    Acknowledgement acknowledgement{
        .id = id
    };
    return acknowledgement;
}

void BinarySerializer::serialize_rejection(const Rejection& rejection, std::vector<uint8_t>& buffer){
    size_t reason_length = rejection.reason.size();
    //id: 8bytes, size of string: 4bytes, the string itself: size defined by size of string previous field
    //PAYLOAD_SIZE cant be constexpr at reason_length is not known at compile time, its known at runtime
    size_t PAYLOAD_SIZE = sizeof(uint64_t) + sizeof(uint32_t) + reason_length;
    size_t offset = buffer.size();
    buffer.resize(offset + 5 + PAYLOAD_SIZE);

    MessageType type = MessageType::REJECT;
    std::memcpy(buffer.data() + offset, &type, sizeof(MessageType));
    offset+=sizeof(MessageType);

    uint32_t net_length = htonl(static_cast<uint32_t>(PAYLOAD_SIZE));
    std::memcpy(buffer.data() + offset, &net_length, sizeof(uint32_t));
    offset+=sizeof(uint32_t);

    uint64_t id = rejection.id;
    std::memcpy(buffer.data() + offset, &id, sizeof(uint64_t));
    offset+=sizeof(uint64_t);

    std::memcpy(buffer.data() + offset, &reason_length, sizeof(uint32_t));
    offset+=sizeof(uint32_t);

    //copy from the data segment of the string object, cant do just &reason as that points to start of string object, not necessarily the data segment
    std::memcpy(buffer.data() + offset, rejection.reason.data(), reason_length);


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

void BinarySerializer::serialize_trade(const Trade& trade, std::vector<uint8_t>& buffer){
    constexpr size_t PAYLOAD_SIZE = sizeof(int64_t) + sizeof(uint64_t) + sizeof(uint64_t)
                        + sizeof(uint32_t) + sizeof(uint64_t);
    size_t offset = buffer.size();
    buffer.resize(offset + 5 + PAYLOAD_SIZE);
    
    MessageType type = MessageType::TRADE;
    std::memcpy(buffer.data() + offset, &type, sizeof(MessageType));
    offset+=sizeof(MessageType);

    uint32_t net_length = htonl(static_cast<uint32_t>(PAYLOAD_SIZE));
    std::memcpy(buffer.data() + offset, &net_length, sizeof(uint32_t));
    offset+=sizeof(uint32_t);

    std::memcpy(buffer.data() + offset, &trade.price, sizeof(int64_t));
    offset+=sizeof(int64_t);

    std::memcpy(buffer.data() + offset, &trade.buyer_order_id, sizeof(uint64_t));
    offset+=sizeof(uint64_t);

    std::memcpy(buffer.data() + offset, &trade.seller_order_id, sizeof(uint64_t));
    offset+=sizeof(uint64_t);

    std::memcpy(buffer.data() + offset, &trade.quantity, sizeof(uint32_t));
    offset+=sizeof(uint32_t);

    std::memcpy(buffer.data() + offset, &trade.timestamp, sizeof(uint64_t));
}  

Trade BinarySerializer::deserialize_trade(const Message& message){
    size_t offset = 0;

    int64_t price;
    std::memcpy(&price, message.payload.data() + offset, sizeof(int64_t));
    offset+=sizeof(int64_t);

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

void BinarySerializer::serialize_cancel_ack(const Acknowledgement& acknowledgement, std::vector<uint8_t>& buffer){
    constexpr size_t PAYLOAD_SIZE = sizeof(uint64_t);
    size_t offset = buffer.size();
    buffer.resize(offset + 5 + PAYLOAD_SIZE);

    MessageType type = MessageType::CANCEL_ACK;
    std::memcpy(buffer.data() + offset, &type, sizeof(MessageType));
    offset+=sizeof(MessageType);

    uint32_t net_length = htonl(static_cast<uint32_t>(PAYLOAD_SIZE));
    std::memcpy(buffer.data() + offset, &net_length, sizeof(uint32_t));
    offset+=sizeof(uint32_t);

    std::memcpy(buffer.data() + offset, &acknowledgement.id, sizeof(uint64_t));
    
}

Acknowledgement BinarySerializer::deserialize_cancel_ack(const Message& message){
uint64_t id;
std::memcpy(&id, message.payload.data(), sizeof(uint64_t));
Acknowledgement acknowledgement{
    .id = id
};
return acknowledgement;
}
