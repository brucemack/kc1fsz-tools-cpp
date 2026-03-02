/**
 * Copyright (C) 2026, Bruce MacKinnon KC1FSZ
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

#include <concepts>

// ### REMOVE
#include <iostream>

#include "kc1fsz-tools/CircularQueuePointers.h"

using namespace std;

namespace kc1fsz {

/**
 * A circular queue that has the feature that popping isn't allowed
 * until the queue has filled beyond a certain threshold. This is
 * useful in rate-matched queues where we want to allow the writer
 * to get a bit ahead of the reader to avoid subtle timing issues.
 */
template<typename T> class CircularQueueWithTrigger {
public:

    /**
     * @param fillTrigger The queue needs to have this many items before 
     * popping is allowed to start.
     */
    CircularQueueWithTrigger(T* space, unsigned spaceLen, unsigned fillTrigger)
    :   _space(space),
        _ptrs(spaceLen),
        _fillTrigger(fillTrigger) { }

    void push(const T* frame, unsigned frameLen) {
        for (unsigned i = 0; i < frameLen; i++) 
            _space[_ptrs.writePtrThenPush()] = frame[i];
    }

    // Specializations for speed for certain types
    // Here we use the max contiguous concept to batch the pushes. If the 
    // push wraps around the end of the circular space then we'll use two pushes.
    void pushInt32(const int32_t* frame, unsigned frameLen) {
        unsigned firstPart = std::min(_ptrs.getMaxContiguousPushLength(), frameLen);
        if (firstPart)
            memcpy(_space + _ptrs.writePtr(), frame, sizeof(int32_t) * firstPart);
        _ptrs.push(firstPart);
        if (!_ptrs.isFull() && firstPart < frameLen) {
            unsigned secondPart = std::min(_ptrs.getMaxContiguousPushLength(), frameLen - firstPart);
            if (secondPart)
                memcpy(_space + _ptrs.writePtr(), frame + firstPart, sizeof(int32_t) * secondPart);
            _ptrs.push(secondPart);
        }
    }

    bool tryPop(T* frame, unsigned frameLen) {
        if (_ptrs.getDepth() < frameLen) {
            _triggered = false;
            return false;
        }
        if (!_triggered) 
            if (_ptrs.getDepth() < _fillTrigger) 
                return false;
        _triggered = true;
        for (unsigned i = 0; i < frameLen; i++) 
            frame[i] = _space[_ptrs.readPtrThenPop()];
        return true;
    }

    unsigned getDepth() const { return _ptrs.getDepth(); }

private:

    T* _space;
    CircularQueuePointers _ptrs;
    unsigned _fillTrigger;
    bool _triggered = false;
};


}
