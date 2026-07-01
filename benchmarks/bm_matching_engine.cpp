#include <benchmark/benchmark.h>
#include "matching_engine.hpp"

static void BM_MatchOrdersAtLevel(benchmark::State& state){
    OrderBook book;
    MatchingEngine matching_engine(book);
    std::vector<Trade> trades;//maybe should reserve this space
    
    int64_t leveldepth = state.range(0);//number of orders at given level

    for(auto _ : state){
        state.PauseTiming();
        int64_t id = 1;
        for(int i = 0; i < leveldepth; i++){
                book.add_order(Order(id++, 1000, 100, Side::BID, OrderType::LIMIT));
            }
        state.ResumeTiming();
        matching_engine.match(Order(id++, 1000, leveldepth*100, Side::ASK, OrderType::LIMIT), trades);
    }

}

BENCHMARK(BM_MatchOrdersAtLevel)->RangeMultiplier(10)->Range(10, 100000);

static void BM_MatchOrdersAcrossLevels(benchmark::State& state){
    OrderBook book;
    MatchingEngine matching_engine(book);
    std::vector<Trade> trades;

    int64_t levels = state.range(0);
    for(auto _ : state){
        state.PauseTiming();
        int64_t id = 1;
        for(int i = 0; i < levels; i++){
            book.add_order(Order(id++, 1000 + i, 100, Side::BID, OrderType::LIMIT));
            //price changes per level, goes across levels
        }
        state.ResumeTiming();
        matching_engine.match(Order(id++, 1000, levels*100, Side::ASK, OrderType::LIMIT), trades);
    }
}

BENCHMARK(BM_MatchOrdersAcrossLevels)->RangeMultiplier(10)->Range(10, 100000);

static void BM_MatchOrdersAcrossByLevelAndAcrossLevels(benchmark::State& state){
    OrderBook book;
    MatchingEngine matching_engine(book);
    std::vector<Trade> trades;

    int64_t leveldepth = state.range(0);
    int64_t levels = state.range(1);
    
    for(auto _ : state){
        state.PauseTiming();
        int64_t id = 1;
        for(int i = 0; i < levels; i++){
            for(int j = 0; j < leveldepth; j++){
                book.add_order(Order(id++, 1000 + i, 100, Side::BID, OrderType::LIMIT));
            }
        }
        state.ResumeTiming();
        matching_engine.match(Order(id++, 1000, 100*leveldepth*levels, Side::ASK, OrderType::LIMIT), trades);
    }
}

BENCHMARK(BM_MatchOrdersAcrossByLevelAndAcrossLevels)->RangeMultiplier(10)->Ranges({{10, 1000}, {10, 1000}});


static void BM_MatchNoFill(benchmark::State& state) {
    OrderBook book;
    MatchingEngine matching_engine(book);
    std::vector<Trade> trades;
    uint64_t id = 1;
    // ask at 1010, bids below won't cross
    book.add_order(Order(id++, 1010, 100, Side::ASK, OrderType::LIMIT));
    for (auto _ : state) {
        matching_engine.match(Order(id, 1000, 100, Side::BID, OrderType::LIMIT), trades);
        book.cancel_order(id);  // clean up so book doesn't grow, incurs extra cost in benchmark
        //assumed to be around 50ns per operation
        //when benchmarked using .pauseTiming and .resumeTiming values were too instable, overhead of pausing brought value to ~500ns
        id++;
    }
}
BENCHMARK(BM_MatchNoFill);


static void BM_MatchSingleFill(benchmark::State& state) {
    OrderBook book;
    MatchingEngine matching_engine(book);
    std::vector<Trade> trades;
    uint64_t id = 1;
    for (auto _ : state) {
        matching_engine.match(Order(id++, 1000, 100, Side::ASK, OrderType::LIMIT), trades);
        matching_engine.match(Order(id++, 1000, 100, Side::BID, OrderType::LIMIT), trades);
    }
}
BENCHMARK(BM_MatchSingleFill);

static void BM_MatchPartialFill(benchmark::State& state) {
    OrderBook book;
    MatchingEngine matching_engine(book);
    std::vector<Trade> trades;
    uint64_t id = 1;
    for (auto _ : state) {
        book.add_order(Order(id++, 1000, 1000, Side::ASK, OrderType::LIMIT));
        matching_engine.match(Order(id++, 1000, 100, Side::BID, OrderType::LIMIT), trades);
    }
}
BENCHMARK(BM_MatchPartialFill);