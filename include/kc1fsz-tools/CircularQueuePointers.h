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
        _readPtr = 0; _writePtr = 0; _fault = false; _depth = 0;
    }

    bool isEmpty() const {
        return _readPtr == _writePtr;
    }

    bool isFull() const {
        return _next(_writePtr) == _readPtr;
    }

    /**
     * @return The current number of items on the queue
     */
    unsigned getDepth() const {
        return _depth;
    }

    bool isFault() const { return _fault; }

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
            _fault = true;
        else {
            _writePtr = _next(_writePtr);
            _depth++;
        }
    }

    void pop() {
        if (isEmpty())
            _fault = true;
        else {
            _readPtr = _next(_readPtr);
            _depth--;
        }
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
    unsigned _readPtr = 0;
    unsigned _writePtr = 0;
    bool _fault = false;
    unsigned _depth = 0;
};

}
