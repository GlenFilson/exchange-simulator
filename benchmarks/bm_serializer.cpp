#include <benchmark/benchmark.h>
#include "binary_serializer.hpp"
#include "order.hpp"
#include "message.hpp"
#include "acknowledgement.hpp"
#include "trade.hpp"


void static BM_BinarySerializerSerializeOrder(benchmark::State& state){
    std::vector<uint8_t> buffer;
    BinarySerializer serializer;
    Order order{1, 1000, 100, Side::BID, OrderType::LIMIT};
    for(auto _ : state){
        buffer.clear();
        serializer.serialize_order(order, buffer);
    }
}

BENCHMARK(BM_BinarySerializerSerializeOrder);

void static BM_BinarySerializerDeserializeOrder(benchmark::State& state){
    std::vector<uint8_t> buffer;
    BinarySerializer serializer;
    Order order{1, 1000, 100, Side::BID, OrderType::LIMIT};
    serializer.serialize_order(order, buffer);

    Message msg;
    msg.type = MessageType::NEW_ORDER;
    //+5 to skip the message type (1bytes) and payload length (4bytes)
    msg.payload.assign(buffer.begin()+5, buffer.end());
    for(auto _ : state){
        benchmark::DoNotOptimize(serializer.deserialize_order(msg));
    }   
}
BENCHMARK(BM_BinarySerializerDeserializeOrder);

void static BM_BinarySerializerSerializeCancel(benchmark::State& state){
    BinarySerializer serializer;
    std::vector<uint8_t> buffer;
    int64_t id = 1;

    for(auto _ : state){
        buffer.clear();
        serializer.serialize_cancel(id++, buffer);
    }
}

BENCHMARK(BM_BinarySerializerSerializeCancel);

void static BM_BinarySerializerDeserializeCancel(benchmark::State& state){
    BinarySerializer serializer;
    std::vector<uint8_t> buffer;
    int64_t id = 1;
    serializer.serialize_cancel(id, buffer);

    Message msg;
    msg.type = MessageType::CANCEL_ORDER;
    msg.payload.assign(buffer.begin()+5, buffer.end());
    for(auto _ : state){
        benchmark::DoNotOptimize(serializer.deserialize_cancel(msg));
    }

}
BENCHMARK(BM_BinarySerializerDeserializeCancel);

void static BM_BinarySerializerSerializeAck(benchmark::State& state){
    BinarySerializer serializer;
    std::vector<uint8_t> buffer;
    Acknowledgement ack{1};
    for(auto _ : state){
        buffer.clear();
        serializer.serialize_acknowledgement(ack, buffer);
    }
}

BENCHMARK(BM_BinarySerializerSerializeAck);

void static BM_BinarySerializerDeserializeAck(benchmark::State& state){
    BinarySerializer serializer;
    std::vector<uint8_t> buffer;
    Acknowledgement ack{1};
    serializer.serialize_acknowledgement(ack, buffer);

    Message msg;
    msg.type = MessageType::ORDER_ACK;
    msg.payload.assign(buffer.begin()+5, buffer.end());

    for(auto _ : state){
        benchmark::DoNotOptimize(serializer.deserialize_acknowledgement(msg));
    }
}
BENCHMARK(BM_BinarySerializerDeserializeAck);

void static BM_BinarySerializerSerializeRejection(benchmark::State& state){
    BinarySerializer serializer;
    std::vector<uint8_t> buffer;
    Rejection rejection{1, std::string()};
    for(auto _ : state){
        buffer.clear();
        serializer.serialize_rejection(rejection, buffer);
    }
}
BENCHMARK(BM_BinarySerializerSerializeRejection);

void static BM_BinarySerializerDeserializeRejection(benchmark::State& state){
    BinarySerializer serializer;
    std::vector<uint8_t> buffer;
    Rejection rejection{1, std::string()};
    serializer.serialize_rejection(rejection, buffer);

    Message msg;
    msg.type = MessageType::REJECT;
    msg.payload.assign(buffer.begin()+5, buffer.end());

    for(auto _ : state){
        benchmark::DoNotOptimize(serializer.deserialize_rejection(msg));
    }
}
BENCHMARK(BM_BinarySerializerDeserializeRejection);

void static BM_BinarySerializerSerializeTrade(benchmark::State& state){
    BinarySerializer serializer;
    std::vector<uint8_t> buffer;

    Trade trade{1000, 1, 2, 100, 12345};
    for(auto _ : state){
        buffer.clear();
        serializer.serialize_trade(trade, buffer);
    }
    
}
BENCHMARK(BM_BinarySerializerSerializeTrade);


void static BM_BinarySerializerDeserializeTrade(benchmark::State& state){
    BinarySerializer serializer;
    std::vector<uint8_t> buffer;
    Trade trade{1000, 1, 2, 100, 12345};
    serializer.serialize_trade(trade, buffer);

    Message msg;
    msg.type = MessageType::TRADE;
    msg.payload.assign(buffer.begin()+5, buffer.end());
    for(auto _ : state){
        benchmark::DoNotOptimize(serializer.deserialize_trade(msg));
    }
}
BENCHMARK(BM_BinarySerializerDeserializeTrade);

void static BM_BinarySerializerSerializeCancelAck(benchmark::State& state){
    BinarySerializer serializer;
    std::vector<uint8_t> buffer;
    Acknowledgement ack{1};
    for(auto _ : state){
        buffer.clear();
        serializer.serialize_cancel_ack(ack, buffer);
    }
}
BENCHMARK(BM_BinarySerializerSerializeCancelAck);

void static BM_BinarySerializerDeserializeCancelAck(benchmark::State& state){
    BinarySerializer serializer;
    std::vector<uint8_t> buffer;
    Acknowledgement ack{1};
    serializer.serialize_cancel_ack(ack, buffer);

    Message msg;
    msg.type = MessageType::CANCEL_ACK;
    msg.payload.assign(buffer.begin()+5, buffer.end());
    for(auto _ : state){
        benchmark::DoNotOptimize(serializer.deserialize_cancel_ack(msg));
    }
}
BENCHMARK(BM_BinarySerializerDeserializeCancelAck);