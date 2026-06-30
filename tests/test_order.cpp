#include <gtest/gtest.h>
#include "order.hpp"

// construction
TEST(OrderTest, ConstructLimitOrder) {
    Order order(1, 1000, 500, Side::BID, OrderType::LIMIT);
    EXPECT_EQ(order.id(), 1);
    EXPECT_EQ(order.price(), 1000);
    EXPECT_EQ(order.quantity(), 500);
    EXPECT_EQ(order.side(), Side::BID);
    EXPECT_EQ(order.orderType(), OrderType::LIMIT);
}

TEST(OrderTest, ConstructMarketOrder) {
    Order order(1, 0, 100, Side::ASK, OrderType::MARKET);
    EXPECT_EQ(order.price(), 0);
    EXPECT_EQ(order.orderType(), OrderType::MARKET);
}

TEST(OrderTest, TimestampIsSet) {
    Order order(1, 1000, 100, Side::BID, OrderType::LIMIT);
    EXPECT_GT(order.timestamp(), 0);
}

// validation
TEST(OrderTest, RejectZeroQuantity) {
    EXPECT_THROW(Order(1, 1000, 0, Side::BID, OrderType::LIMIT), std::invalid_argument);
}

TEST(OrderTest, RejectLimitZeroPrice) {
    EXPECT_THROW(Order(1, 0, 100, Side::BID, OrderType::LIMIT), std::invalid_argument);
}

TEST(OrderTest, RejectLimitNegativePrice) {
    EXPECT_THROW(Order(1, -500, 100, Side::BID, OrderType::LIMIT), std::invalid_argument);
}

TEST(OrderTest, RejectMarketWithPrice) {
    EXPECT_THROW(Order(1, 1000, 100, Side::BID, OrderType::MARKET), std::invalid_argument);
}

// reducing quantity
TEST(OrderTest, ReduceQuantity) {
    Order order(1, 1000, 100, Side::BID, OrderType::LIMIT);
    order.reduce_quantity(30);
    EXPECT_EQ(order.quantity(), 70);
}

TEST(OrderTest, ReduceQuantityToZero) {
    Order order(1, 1000, 100, Side::BID, OrderType::LIMIT);
    order.reduce_quantity(100);
    EXPECT_EQ(order.quantity(), 0);
}

TEST(OrderTest, ReduceQuantityTooMuch) {
    Order order(1, 1000, 100, Side::BID, OrderType::LIMIT);
    EXPECT_THROW(order.reduce_quantity(101), std::invalid_argument);
}