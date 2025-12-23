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

#include <thread>
#include <mutex>
#include <functional>

template <typename T>
class copyableatomic {
public:

    copyableatomic() = default;

    void set(T newValue) {
        std::lock_guard<std::mutex> lk(mut);
        _safeValue = newValue;
    }
   
    T getCopy() const {
        std::lock_guard<std::mutex> lk(mut);
        T c = _safeValue;
        return c;
    }

    /**
     * The callback provides access to the live value while under 
     * the control of the lock.
     */
    void manipulateUnderLock(std::function<void(T& liveValue)> cb) {
        std::lock_guard<std::mutex> lk(mut);
        cb(_safeValue);
    }

private:

    mutable std::mutex mut;
    T _safeValue;
};
