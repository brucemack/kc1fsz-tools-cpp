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

namespace kc1fsz {

/**
 * A generic class for managing the pointers in a circular queue.
 * (I'm tired of writing this over an over again.)
 */
class CircularQueuePointers {
public:
    
    CircularQueuePointers(unsigned capacity)
    :   _capacity(capacity) {        
        reset();
    }

    void reset() {
        _readPtr = 0; _writePtr = 0; _overflowCount = 0; _underflowCount = 0; _depth = 0;
    }

    bool isEmpty() const {
        return _readPtr == _writePtr;
    }

    bool isFull() const {
        // Hold back one to avoid the full/empty ambiguity
        return _next(_writePtr) == _readPtr;
    }

    /**
     * @return The current number of items on the queue
     */
    unsigned getDepth() const {
        return _depth;
    }

    /**
     * @return The number of free spaces in the queue.
     */
    unsigned getFree() const {
        // Hold back one to avoid the full/empty ambiguity
        return _capacity - getDepth() - 1;
    }

    bool isFault() const { return _overflowCount != 0 || _underflowCount != 0; }

    unsigned getOverflows() const { return _overflowCount; }

    unsigned getUnderflows() const { return _underflowCount; }

    unsigned writePtr() const { return _writePtr; }

    unsigned writePtrThenPush() { 
        unsigned t = _writePtr; 
        push(); 
        return t;
    }

    unsigned readPtr() const { return _readPtr; }

    unsigned readPtrThenPop() { 
        unsigned t = _readPtr; 
        pop(); 
        return t;
    }

    void push() { 
        if (isFull())
            _overflowCount = _overflowCount + 1;
        else {
            _writePtr = _next(_writePtr);
            _depth = _depth + 1;
        }
    }

    void push(unsigned c) { 
        const unsigned p = std::min(c, getFree());
        _writePtr = _writePtr + p;
        if (_writePtr >= _capacity)
            _writePtr = _writePtr - _capacity;
        _depth = _depth + p;
        if (p < c)
            _overflowCount = _overflowCount + 1;
    }

    void pop() {
        if (isEmpty())
            _underflowCount = _underflowCount + 1;
        else {
            _readPtr = _next(_readPtr);
            _depth = _depth - 1;
        }
    }

    void pop(unsigned c) { 
        const unsigned p = std::min(c, getDepth());
        _readPtr = _readPtr + p;
        // Manage wrap
        if (_readPtr >= _capacity)
            _readPtr = _readPtr - _capacity;
        _depth = _depth - p;
        if (p < c)
            _underflowCount = _underflowCount + 1;
    }

    /**
     * @returns The largest contiguous write that can be performed before 
     * overflowing or wrapping.
     */
    unsigned getMaxContiguousPushLength() const {
        if (isFull()) 
            return 0;
        // When read pointer is to the right of the write pointer then we
        // can only write as far as the reader pointer - 1 before filling up.
        else if (_readPtr > _writePtr)
            return _readPtr - _writePtr - 1;
        // When the read pointer is to the left then we can write as much 
        // as the capacity will allow, keeping one free to avoid the full/empty
        // ambiguity.
        else 
            return std::min(_capacity - _writePtr, _capacity - 1);
    }

    unsigned getMaxContiguousPopLength() const {
        if (isEmpty()) 
            return 0;
        // When write pointer is to the right of the read pointer then we
        // can only read as far as the write pointer before emptying out.
        else if (_writePtr > _readPtr)
            return _writePtr - _readPtr;
        // When the write pointer is to the left then we can read as much 
        // as the capacity will allow.
        else 
            return _capacity - _readPtr;
    }

private:

    /**
     * Increments the value, wrapping if needed
     */
    unsigned _next(unsigned i) const {
        i++;
        if (i == _capacity)
            i = 0;
        return i;
    }

    const unsigned _capacity;
    volatile unsigned _readPtr = 0;
    volatile unsigned _writePtr = 0;
    volatile unsigned _overflowCount = 0;
    volatile unsigned _underflowCount = 0;
    volatile unsigned _depth = 0;
};

}
