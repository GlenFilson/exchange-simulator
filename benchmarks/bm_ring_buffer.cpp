#include <benchmark/benchmark.h>
#include "spsc_ring_buffer.hpp"
#include "message.hpp"
#include "inbound_message.hpp"
#include "order.hpp"
#include "binary_serializer.hpp"



static void BM_RingBufferPushPop(benchmark::State& state){
    SPSCRingBuffer<int, 1024> buffer;
    int item = 123;
    for(auto _ : state){
        buffer.try_push(item);
        benchmark::DoNotOptimize(buffer.try_pop());
    }
}
BENCHMARK(BM_RingBufferPushPop);

static void BM_RingBufferPushPopInboundMessage(benchmark::State& state) {
    SPSCRingBuffer<InboundMessage, 1024> buffer;
    BinarySerializer serializer;
    InboundMessage msg{};
    msg.fd = 5;
    msg.type = MessageType::NEW_ORDER;
    Order order{1, 1000, 100, Side::BID, OrderType::LIMIT};
    std::vector<uint8_t> buffer2;
    serializer.serialize_order(order, buffer2);
    msg.payload.assign(buffer2.begin(), buffer2.end());
    
    for (auto _ : state) {
        buffer.try_push(msg);
        benchmark::DoNotOptimize(buffer.try_pop());
    }
}
BENCHMARK(BM_RingBufferPushPopInboundMessage);

static void BM_RingBufferSize64(benchmark::State& state) {
    SPSCRingBuffer<int, 64> buffer;
    int item = 123;
    for (auto _ : state) {
        buffer.try_push(item);
        benchmark::DoNotOptimize(buffer.try_pop());
    }
}
BENCHMARK(BM_RingBufferSize64);

static void BM_RingBufferSize8192(benchmark::State& state) {
    SPSCRingBuffer<int, 8192> buffer;
    int item = 123;
    for (auto _ : state) {
        buffer.try_push(item);
        benchmark::DoNotOptimize(buffer.try_pop());
    }
}
BENCHMARK(BM_RingBufferSize8192);

static void BM_RingBufferPushFull(benchmark::State& state) {
    SPSCRingBuffer<int, 64> buffer;
    // fill it
    for (int i = 0; i < 64; i++) buffer.try_push(i);
    for (auto _ : state) {
        benchmark::DoNotOptimize(buffer.try_push(999));
    }
}
BENCHMARK(BM_RingBufferPushFull);

static void BM_RingBufferPopEmpty(benchmark::State& state) {
    SPSCRingBuffer<int, 64> buffer;
    for (auto _ : state) {
        benchmark::DoNotOptimize(buffer.try_pop());
    }
}
BENCHMARK(BM_RingBufferPopEmpty);