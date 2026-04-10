#include <gtest/gtest.h>
#include "spsc_ring_buffer.hpp"

TEST(RingBufferTest, PopFromEmptyBuffer){
    SPSCRingBuffer<int, 8> buffer;
    EXPECT_EQ(buffer.try_pop(), std::nullopt);
}


TEST(RingBufferTest, PushThenPop){
    SPSCRingBuffer<int, 8> buffer;
    buffer.try_push(5);
    EXPECT_EQ(buffer.try_pop(), 5);
}

TEST(RingBufferTest, PushToFullBuffer){
    SPSCRingBuffer<int, 4> buffer;
    buffer.try_push(1);
    buffer.try_push(2);
    buffer.try_push(3);
    buffer.try_push(4);
    EXPECT_EQ(buffer.try_push(5), false);
}


TEST(RingBufferTest, FIFOOrder){
    SPSCRingBuffer<int, 8> buffer;
    buffer.try_push(1);
    buffer.try_push(2);
    buffer.try_push(3);
    buffer.try_push(4);
    EXPECT_EQ(buffer.try_pop(), 1);
    EXPECT_EQ(buffer.try_pop(), 2);
    EXPECT_EQ(buffer.try_pop(), 3);
    EXPECT_EQ(buffer.try_pop(), 4);
}

TEST(RingBufferTest, PushPopInterleaved){
    SPSCRingBuffer<int, 4> buffer;
    buffer.try_push(1);
    buffer.try_push(2);
    buffer.try_push(3);

    EXPECT_EQ(buffer.try_pop(), 1);
    EXPECT_EQ(buffer.try_pop(), 2);

    buffer.try_push(4);
    buffer.try_push(5);
    buffer.try_push(6);

    EXPECT_EQ(buffer.try_pop(), 3);
    EXPECT_EQ(buffer.try_pop(), 4);
    EXPECT_EQ(buffer.try_pop(), 5);
    EXPECT_EQ(buffer.try_pop(), 6);


}

TEST(RingBufferTest, FillThenFlush){
    SPSCRingBuffer<int, 4> buffer;
    buffer.try_push(1);
    buffer.try_push(2);
    buffer.try_push(3);
    buffer.try_push(4);

    EXPECT_EQ(buffer.try_pop(), 1);
    EXPECT_EQ(buffer.try_pop(), 2);
    EXPECT_EQ(buffer.try_pop(), 3);
    EXPECT_EQ(buffer.try_pop(), 4);

    buffer.try_push(5);
    buffer.try_push(6);
    buffer.try_push(7);
    buffer.try_push(8);

    EXPECT_EQ(buffer.try_pop(), 5);
    EXPECT_EQ(buffer.try_pop(), 6);
    EXPECT_EQ(buffer.try_pop(), 7);
    EXPECT_EQ(buffer.try_pop(), 8);
}