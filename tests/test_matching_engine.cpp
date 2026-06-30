#include <gtest/gtest.h>
#include "orderbook.hpp"
#include "matching_engine.hpp"
#include "trade.hpp"

TEST(MatchingEngineTest, FullFill){
    OrderBook orderbook;
    MatchingEngine matching_engine(orderbook);
    std::vector<Trade> trades;

    Order order1(1, 100, 2, Side::ASK, OrderType::LIMIT);
    matching_engine.match(order1, trades);
    EXPECT_EQ(trades.size(), 0);//trade not matched, sitting on book
    EXPECT_EQ(orderbook.best_ask(), 100);

    Order order2(2, 100, 1, Side::BID, OrderType::LIMIT);//should get fully filled
    matching_engine.match(order2, trades);
    EXPECT_EQ(trades.size(), 1);
    //book should still have resting ask at 100
    EXPECT_EQ(orderbook.best_ask(), 100);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].quantity, 1);
    EXPECT_EQ(trades[0].buyer_order_id, 2);//taker
    EXPECT_EQ(trades[0].seller_order_id, 1);//maker, order was resting initially   
}

TEST(MatchingEngineTest, PartialFill){
    OrderBook orderbook;
    MatchingEngine matching_engine(orderbook);
    std::vector<Trade> trades;

    Order order1(1, 100, 50, Side::ASK, OrderType::LIMIT);
    matching_engine.match(order1, trades);
    EXPECT_EQ(trades.size(), 0);
    EXPECT_EQ(orderbook.best_ask(), 100);

    Order order2(2, 100, 100, Side::BID, OrderType::LIMIT);//order is 2x size of best ask, should get 50 filled
    matching_engine.match(order2, trades);
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(orderbook.best_ask(), std::nullopt);//wiped out asks
    EXPECT_EQ(orderbook.best_bid(), 100);//remaining bid sits on book
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].quantity, 50);//got partial filled for the 50 ask sitting
    EXPECT_EQ(trades[0].buyer_order_id, 2);
    EXPECT_EQ(trades[0].seller_order_id, 1);
}

TEST(MatchingEngineTest, NoMatch){
    OrderBook orderbook;
    MatchingEngine matching_engine(orderbook);
    std::vector<Trade> trades;

    Order order1(1, 100, 10, Side::ASK, OrderType::LIMIT);
    matching_engine.match(order1, trades);
    
    Order order2(2, 95, 10, Side::BID, OrderType::LIMIT);
    matching_engine.match(order2, trades);
    
    EXPECT_EQ(trades.size(), 0);
    EXPECT_EQ(orderbook.best_ask(), 100);
    EXPECT_EQ(orderbook.best_bid(), 95);
}

TEST(MatchingEngineTest, MarketOrder){
    OrderBook orderbook;
    MatchingEngine matching_engine(orderbook);
    std::vector<Trade> trades;

    Order order1(1, 100, 10, Side::ASK, OrderType::LIMIT);
    matching_engine.match(order1, trades);

    Order order2(2, 0, 10, Side::BID, OrderType::MARKET);
    matching_engine.match(order2, trades);//should fill at best price, 100
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].quantity, 10);
    EXPECT_EQ(trades[0].buyer_order_id, 2);
    EXPECT_EQ(trades[0].seller_order_id, 1);

}

TEST(MatchingEngineTest, MarketOrderEmptyBook){
    OrderBook orderbook;
    MatchingEngine matching_engine(orderbook);
    std::vector<Trade> trades;

    Order order1(1, 0, 100, Side::BID, OrderType::MARKET);
    matching_engine.match(order1, trades);
    EXPECT_EQ(trades.size(), 0);
    EXPECT_EQ(orderbook.best_ask(), std::nullopt);
    EXPECT_EQ(orderbook.best_bid(), std::nullopt);
}

TEST(MatchingEngineTest, MarketOrderPartialFill){
    OrderBook orderbook;
    MatchingEngine matching_engine(orderbook);
    std::vector<Trade> trades;

    Order order1(1, 100, 50, Side::ASK, OrderType::LIMIT);
    matching_engine.match(order1, trades);

    Order order2(2, 0, 100, Side::BID, OrderType::MARKET);
    matching_engine.match(order2, trades);
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].quantity, 50);
    EXPECT_EQ(trades[0].buyer_order_id, 2);
    EXPECT_EQ(trades[0].seller_order_id, 1);
    //also check nothing left over, market order partial filled, wiped out ask
    EXPECT_EQ(orderbook.best_bid(), std::nullopt);
    EXPECT_EQ(orderbook.best_ask(), std::nullopt);
}

TEST(MatchingEngineTest, PriceTimePriority){
    OrderBook orderbook;
    MatchingEngine matching_engine(orderbook);
    std::vector<Trade> trades;

    Order order1(1, 100, 50, Side::ASK, OrderType::LIMIT);
    Order order2(2, 100, 50, Side::ASK, OrderType::LIMIT);
    matching_engine.match(order1, trades);
    matching_engine.match(order2, trades);

    Order order3(3, 100, 50, Side::BID, OrderType::LIMIT);
    matching_engine.match(order3, trades);
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].quantity, 50);
    EXPECT_EQ(trades[0].seller_order_id, 1);//should fill the first order, time priority
}

TEST(MatchingEngineTest, PriceTimePriorityMultipleTrades){
    OrderBook orderbook;
    MatchingEngine matching_engine(orderbook);
    std::vector<Trade> trades;

    Order order1(1, 100, 50, Side::ASK, OrderType::LIMIT);
    Order order2(2, 100, 50, Side::ASK, OrderType::LIMIT);
    matching_engine.match(order1, trades);
    matching_engine.match(order2, trades);

    Order order3(3, 100, 100, Side::BID, OrderType::LIMIT);
    matching_engine.match(order3, trades);
    EXPECT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].quantity, 50);
    EXPECT_EQ(trades[0].seller_order_id, 1);//should fill the first order, time priority

    EXPECT_EQ(trades[1].price, 100);
    EXPECT_EQ(trades[1].quantity, 50);
    EXPECT_EQ(trades[1].seller_order_id, 2);

    //book should now be empty
    EXPECT_EQ(orderbook.best_bid(), std::nullopt);
    EXPECT_EQ(orderbook.best_ask(), std::nullopt);

}

TEST(MatchingEngineTest, MarketOrderMultipleLevel){
    OrderBook orderbook;
    MatchingEngine matching_engine(orderbook);
    std::vector<Trade> trades;


    Order order1(1, 100, 50, Side::ASK, OrderType::LIMIT);
    Order order2(2, 101, 50, Side::ASK, OrderType::LIMIT);
    Order order3(3, 102, 50, Side::ASK, OrderType::LIMIT);
    matching_engine.match(order1, trades);
    matching_engine.match(order2, trades);
    matching_engine.match(order3, trades);

    Order order4(4, 0, 150, Side::BID, OrderType::MARKET);
    matching_engine.match(order4, trades);
    EXPECT_EQ(trades.size(), 3);//should wipe out all levels
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].seller_order_id, 1);
    EXPECT_EQ(trades[0].quantity, 50);

    EXPECT_EQ(trades[1].price, 101);
    EXPECT_EQ(trades[1].seller_order_id, 2);
    EXPECT_EQ(trades[1].quantity, 50);

    EXPECT_EQ(trades[2].price, 102);
    EXPECT_EQ(trades[2].seller_order_id, 3);
    EXPECT_EQ(trades[2].quantity, 50);

    //book should now be empty
    EXPECT_EQ(orderbook.best_bid(), std::nullopt);
    EXPECT_EQ(orderbook.best_ask(), std::nullopt);

}

TEST(MatchingEngineTest, LimitOrderMultipleLevel){

    OrderBook orderbook;
    MatchingEngine matching_engine(orderbook);
    std::vector<Trade> trades;


    Order order1(1, 100, 50, Side::ASK, OrderType::LIMIT);
    Order order2(2, 101, 50, Side::ASK, OrderType::LIMIT);
    Order order3(3, 102, 50, Side::ASK, OrderType::LIMIT);
    matching_engine.match(order1, trades);
    matching_engine.match(order2, trades);
    matching_engine.match(order3, trades);

    Order order4(4, 102, 150, Side::BID, OrderType::LIMIT);
    matching_engine.match(order4, trades);
    EXPECT_EQ(trades.size(), 3);//should wipe out all levels
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].seller_order_id, 1);
    EXPECT_EQ(trades[0].quantity, 50);

    EXPECT_EQ(trades[1].price, 101);
    EXPECT_EQ(trades[1].seller_order_id, 2);
    EXPECT_EQ(trades[1].quantity, 50);

    EXPECT_EQ(trades[2].price, 102);
    EXPECT_EQ(trades[2].seller_order_id, 3);
    EXPECT_EQ(trades[2].quantity, 50);

    //book should now be empty
    EXPECT_EQ(orderbook.best_bid(), std::nullopt);
    EXPECT_EQ(orderbook.best_ask(), std::nullopt);


}

TEST(MatchingEngineTest, MarketOrderMultipleLevelPartialFill){

    OrderBook orderbook;
    MatchingEngine matching_engine(orderbook);
    std::vector<Trade> trades;


    Order order1(1, 100, 50, Side::ASK, OrderType::LIMIT);
    Order order2(2, 101, 50, Side::ASK, OrderType::LIMIT);
    Order order3(3, 102, 50, Side::ASK, OrderType::LIMIT);
    matching_engine.match(order1, trades);
    matching_engine.match(order2, trades);
    matching_engine.match(order3, trades);

    Order order4(4, 0, 130, Side::BID, OrderType::MARKET);//should do 3 trades, leave 20 remaining at 102
    matching_engine.match(order4, trades);
    EXPECT_EQ(trades.size(), 3);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].seller_order_id, 1);
    EXPECT_EQ(trades[0].quantity, 50);

    EXPECT_EQ(trades[1].price, 101);
    EXPECT_EQ(trades[1].seller_order_id, 2);
    EXPECT_EQ(trades[1].quantity, 50);

    EXPECT_EQ(trades[2].price, 102);
    EXPECT_EQ(trades[2].seller_order_id, 3);
    EXPECT_EQ(trades[2].quantity, 30);


    EXPECT_EQ(orderbook.best_bid(), std::nullopt);
    //didnt wipe out last ask
    EXPECT_EQ(orderbook.best_ask(), 102);

}

