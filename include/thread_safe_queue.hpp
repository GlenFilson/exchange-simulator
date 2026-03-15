#pragma once
#include <queue>
#include <mutex>
#include <optional>

template <typename T>

class ThreadSafeQueue{
    public:

        void push(T item){
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
        };  

        std::optional<T> try_pop(){
            std::lock_guard<std::mutex> lock(mutex_);
            if(queue_.empty()) return std::nullopt;
            T value = std::move(queue_.front());
            queue_.pop();
            return value;
        }
    private: 
        std::queue<T> queue_;
        mutable std::mutex mutex_;
};