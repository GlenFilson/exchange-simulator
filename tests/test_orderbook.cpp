#include <gtest/gtest.h>
#include "orderbook.hpp"
#include "order.hpp"

TEST(OrderBookTest, EmptyBook){
    OrderBook orderbook;
    EXPECT_EQ(orderbook.best_ask(), std::nullopt);
    EXPECT_EQ(orderbook.best_bid(), std::nullopt);
}

TEST(OrderBookTest, EmptyAsk){
    OrderBook orderbook;
    orderbook.add_order(Order(1, 1, 1, Side::BID, OrderType::LIMIT));
    EXPECT_EQ(orderbook.best_ask(), std::nullopt);
    EXPECT_EQ(orderbook.best_bid(), 1);//one bid in
}

TEST(OrderBookTest, EmptyBid){
    OrderBook orderbook;
    orderbook.add_order(Order(1, 1, 1, Side::ASK, OrderType::LIMIT));
    EXPECT_EQ(orderbook.best_ask(), 1);
    EXPECT_EQ(orderbook.best_bid(), std::nullopt);
}

TEST(OrderBookTest, BestAsk){
    OrderBook orderbook;
    orderbook.add_order(Order(1, 103, 1, Side::ASK, OrderType::LIMIT));
    orderbook.add_order(Order(2, 102, 1, Side::ASK, OrderType::LIMIT));
    orderbook.add_order(Order(3, 101, 1, Side::ASK, OrderType::LIMIT));
    orderbook.add_order(Order(4, 100, 1, Side::ASK, OrderType::LIMIT));
    EXPECT_EQ(orderbook.best_ask(), 100);
}

TEST(OrderBookTest, BestBid){
    OrderBook orderbook;
    orderbook.add_order(Order(1, 103, 1, Side::BID, OrderType::LIMIT));
    orderbook.add_order(Order(2, 102, 1, Side::BID, OrderType::LIMIT));
    orderbook.add_order(Order(3, 101, 1, Side::BID, OrderType::LIMIT));
    orderbook.add_order(Order(4, 100, 1, Side::BID, OrderType::LIMIT));
    EXPECT_EQ(orderbook.best_bid(), 103);
}

TEST(OrderBookTest, CancelOrder){
    OrderBook orderbook;
    orderbook.add_order(Order(1, 101, 1, Side::BID, OrderType::LIMIT));
    orderbook.add_order(Order(2, 100, 1, Side::BID, OrderType::LIMIT));

    EXPECT_EQ(orderbook.best_bid(), 101);
    orderbook.cancel_order(1);
    EXPECT_EQ(orderbook.best_bid(), 100);
    orderbook.cancel_order(2);
    EXPECT_EQ(orderbook.best_bid(), std::nullopt);
}


TEST(OrderBookTest, CancelInvalidOrder){
    OrderBook orderbook;
    orderbook.add_order(Order(1, 101, 1, Side::BID, OrderType::LIMIT));
    EXPECT_THROW(orderbook.cancel_order(2), std::invalid_argument);

}

TEST(OrderBookTest, LevelExistsAfterCancel){
    OrderBook orderbook;
    orderbook.add_order(Order(1, 100, 1, Side::BID, OrderType::LIMIT));
    orderbook.add_order(Order(2, 100, 1, Side::BID, OrderType::LIMIT));
    orderbook.cancel_order(1);
    EXPECT_EQ(orderbook.best_bid(), 100);
}



