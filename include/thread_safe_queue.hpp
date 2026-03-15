#pragma once
#include <queue>
#include <mutex>
#include <optional>
#include <atomic>
template <typename T>

class ThreadSafeQueue{
    public:

        void push(T item){
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
            //everything before must be visible to other threads before size is updated
            //means that if a core sees updated size, it is gauranteed to see the item in the queue as well
            size_.fetch_add(1, std::memory_order_release);
        };  

        std::optional<T> try_pop(){
            //ensures that all operations by other threads are visible to this thread, so if size is > 0, we are gauranteed to see the item in the queue as well
            if(size_.load(std::memory_order_acquire) == 0) return std::nullopt;//size check without locking
            std::lock_guard<std::mutex> lock(mutex_);
            T value = std::move(queue_.front());
            queue_.pop();
            //if a thread sees decreased size, it is gauranteed to see the popped item removed from the queue as well
            size_.fetch_sub(1, std::memory_order_release);
            return value;
        }
    private: 
        std::queue<T> queue_;
        mutable std::mutex mutex_;
        std::atomic<size_t> size_{0};
};