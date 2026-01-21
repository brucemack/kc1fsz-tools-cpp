/**
 * Copyright (C) 2025, Bruce MacKinnon KC1FSZ
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once 

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace kc1fsz {

template <typename T> class threadsafequeue2 {
public:

    threadsafequeue2() = default;

    void push(T new_value) {
        std::lock_guard<std::mutex> lk(_mutex);
        bool empty = _queue.empty();
        _queue.push(std::move(new_value));
        // If we going from empty to non-empty wake up one blocker
        if (empty)
            _cond.notify_one();
    }
   
    bool try_pop(T& value, unsigned timeoutMs) {

        // A smart pointer, doesn't actually lock
        std::unique_lock<std::mutex> ulock(_mutex);
        // Wait until the queue is not empty. The wait atomically unlocks the mutex 
        // and re-acquires it upon being notified and before checking the predicate.
        bool success = _cond.wait_for(ulock, 
            std::chrono::milliseconds(timeoutMs),
            [this] { 
                return !_queue.empty(); 
            }
        );
        if (success) {
            value = std::move(_queue.front());
            _queue.pop();
            return true;
        }
        else {
            return false;
        }
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(_mutex);
        return _queue.empty();
    }

    void clear() {
        std::lock_guard<std::mutex> lk(_mutex);
        while (!_queue.empty())
            _queue.pop();
    }

private:

    mutable std::mutex _mutex;
    std::queue<T> _queue;
    std::condition_variable _cond;
};

}
