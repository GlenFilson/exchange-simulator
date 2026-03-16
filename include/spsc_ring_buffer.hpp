#pragma once

#include <optional>
#include <atomic>
#include <array>

/*
size should be declared as a power of 2
this allows us to replace the % operation which is expensive
normally when incrementing the tail we would do:
    tail = (tail + 1) % Capacity
but, if capacity is 2^n, then:
    tail % Capacity == tail & (Capacity - 1)
example:
    13 % 8 = 5
in binary:
    13 = 1101
    7 = 0111 (Capacity - 1)

    1101 & 0111 = 0101 = 5
so, instead of modulo, we can do:
    tail = (tail + 1) & (Capacity - 1)
*/
template <typename T, size_t Capacity>

class SPSCRingBuffer {
    public:
        bool try_push(T item){
            //write is the only one who changes head, can be read relaxed
           size_t head = head_.load(std::memory_order_relaxed);
           size_t tail = tail_.load(std::memory_order_acquire);
            
            //buffer is full
            if(head - tail == Capacity){
                return false;
            }
            //heads position is 1 past the last written item, head points to next item to write to
            buffer_[head & (Capacity - 1)] = std::move(item);

            //increment head to next slot
            head_.store(head + 1, std::memory_order_release);
            return true;
        }

        std::optional<T> try_pop(){
            size_t tail = tail_.load(std::memory_order_relaxed);
            size_t head = head_.load(std::memory_order_acquire);
            //buffer is emoty, nothing to read. consumer has caught up to producer
            if(tail == head){
                return std::nullopt;
            }
            T item = std::move(buffer_[tail & (Capacity - 1)]);
            tail_.store(tail + 1, std::memory_order_release);
            return item;
        }



    private:
        
        std::array<T, Capacity> buffer_;
        std::atomic<size_t> head_{0};
        std::atomic<size_t> tail_{0};
        
        
    
};