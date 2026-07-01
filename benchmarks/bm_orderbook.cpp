#include <benchmark/benchmark.h>
#include "orderbook.hpp"
#include "order.hpp"

static void BM_AddOrder(benchmark::State& state) {
    OrderBook book;
    uint64_t id = 1;
    for (auto _ : state) {
        book.add_order(Order(id++, 1000, 100, Side::BID, OrderType::LIMIT));
    }
}
BENCHMARK(BM_AddOrder);

static void BM_AddOrderSameLevelAtDepth(benchmark::State& state){
    OrderBook book;
    int64_t depth = state.range(0);//uses variable depth, as passed in by BENCHMARK
    for(int i = 0; i < depth; i++){
        book.add_order(Order(i+1, 1000, 100, Side::BID, OrderType::LIMIT));
        //orders are added to same price (1000)
    }
    uint64_t id = depth + 1;
    for(auto _ : state){
        book.add_order(Order(id++, 1000, 100, Side::BID, OrderType::LIMIT));
    }
}
BENCHMARK(BM_AddOrderSameLevelAtDepth)->RangeMultiplier(10)->Range(10, 100000);

static void BM_AddOrderDifferentLevelAtDepth(benchmark::State& state){
    OrderBook book;
    int64_t depth = state.range(0);
    for(int i = 0; i < depth; i++){
        book.add_order(Order(i+1, 1000+i, 100, Side::BID, OrderType::LIMIT));
        //orders are added to different price levels (1000+i)
    }
    uint64_t id = depth + 1;
    for(auto _ : state){
        book.add_order(Order(id++, 1000, 100, Side::BID, OrderType::LIMIT));
    }
}

BENCHMARK(BM_AddOrderDifferentLevelAtDepth)->RangeMultiplier(10)->Range(10, 100000);

static void BM_CancelOrder(benchmark::State& state) {
    OrderBook book;
    int64_t depth = state.range(0);
    for (int i = 0; i < depth; i++) {
        book.add_order(Order(i + 1, 1000, 100, Side::BID, OrderType::LIMIT));
    }
    uint64_t id = depth + 1;
    for (auto _ : state) {
        state.PauseTiming();
        book.add_order(Order(id, 1000, 100, Side::BID, OrderType::LIMIT));
        state.ResumeTiming();
        
        book.cancel_order(id);
        id++;
    }
}
BENCHMARK(BM_CancelOrder)->RangeMultiplier(10)->Range(10, 100000);


static void BM_BestBid(benchmark::State& state) {
    OrderBook book;
    int64_t depth = state.range(0);
    for (int i = 0; i < depth; i++) {
        book.add_order(Order(i + 1, 1000 + i, 100, Side::BID, OrderType::LIMIT));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(book.best_bid());
    }
}
BENCHMARK(BM_BestBid)->RangeMultiplier(10)->Range(10, 100000);

static void BM_BestAsk(benchmark::State& state) {
    OrderBook book;
    int64_t depth = state.range(0);
    for (int i = 0; i < depth; i++) {
        book.add_order(Order(i + 1, 1000 + i, 100, Side::ASK, OrderType::LIMIT));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(book.best_ask());
    }
}
BENCHMARK(BM_BestAsk)->RangeMultiplier(10)->Range(10, 100000);

