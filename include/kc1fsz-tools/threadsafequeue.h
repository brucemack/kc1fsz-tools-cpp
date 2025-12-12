#pragma once 

#include <queue>
#include <thread>
#include <mutex>

template <typename T>
class threadsafequeue {
public:

    threadsafequeue() = default;

    void push(T new_value) {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(std::move(new_value));
    }
   
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return false;
        value = std::move(data_queue.front());
        data_queue.pop();
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }

private:

    mutable std::mutex mut;
    std::queue<T> data_queue;
};
