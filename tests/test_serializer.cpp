#include <gtest/gtest.h>
#include "binary_serializer.hpp"
#include "order.hpp"
#include "trade.hpp"
#include "acknowledgement.hpp"
#include "rejection.hpp"

TEST(SerializerTest, RoundTripOrder) {
    BinarySerializer serializer;
    std::vector<uint8_t> buffer;

    Order original(1, 100, 50, Side::ASK, OrderType::LIMIT);
    serializer.serialize_order(original, buffer);

    Message msg;
    msg.type = MessageType::NEW_ORDER;
    msg.payload.assign(buffer.begin() + 5, buffer.end());

    Order result = serializer.deserialize_order(msg);
    EXPECT_EQ(result.id(), 1);
    EXPECT_EQ(result.price(), 100);
    EXPECT_EQ(result.quantity(), 50);
    EXPECT_EQ(result.side(), Side::ASK);
    EXPECT_EQ(result.orderType(), OrderType::LIMIT);
}

TEST(SerializerTest, RoundTripTrade) {
    BinarySerializer serializer;
    std::vector<uint8_t> buffer;

    Trade original{
        .price = 100,
        .buyer_order_id = 1,
        .seller_order_id = 2,
        .quantity = 50,
        .timestamp = 123456789
    };
    serializer.serialize_trade(original, buffer);

    Message msg;
    msg.type = MessageType::TRADE;
    msg.payload.assign(buffer.begin() + 5, buffer.end());

    Trade result = serializer.deserialize_trade(msg);
    EXPECT_EQ(result.price, 100);
    EXPECT_EQ(result.buyer_order_id, 1);
    EXPECT_EQ(result.seller_order_id, 2);
    EXPECT_EQ(result.quantity, 50);
    EXPECT_EQ(result.timestamp, 123456789);
}

TEST(SerializerTest, RoundTripAck){
    BinarySerializer serializer;
    std::vector<uint8_t> buffer;

    Acknowledgement original{.id = 1};
    serializer.serialize_acknowledgement(original, buffer);

    Message msg;
    msg.type = MessageType::ORDER_ACK;
    msg.payload.assign(buffer.begin() + 5, buffer.end());

    Acknowledgement result = serializer.deserialize_acknowledgement(msg);
    EXPECT_EQ(result.id, 1);
}


TEST(SerializerTest, RoundTripRejection){
    BinarySerializer serializer;
    std::vector<uint8_t> buffer;

    Rejection original{.id = 1, .reason = std::string("rejection reason")};
    serializer.serialize_rejection(original, buffer);

    Message msg;
    msg.type = MessageType::REJECT;
    msg.payload.assign(buffer.begin() + 5, buffer.end());

    Rejection result = serializer.deserialize_rejection(msg);
    EXPECT_EQ(result.id, 1);
    EXPECT_EQ(result.reason, std::string("rejection reason"));
}

TEST(SerializerTest, RoundTripCancel){

    BinarySerializer serializer;
    std::vector<uint8_t> buffer;

    uint64_t original_id = 12345;
    serializer.serialize_cancel(original_id, buffer);

    Message msg;
    msg.type = MessageType::CANCEL_ORDER;
    msg.payload.assign(buffer.begin() + 5, buffer.end());

    uint64_t result_id = serializer.deserialize_cancel(msg);
    EXPECT_EQ(result_id, 12345);


}