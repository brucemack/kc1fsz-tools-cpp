/**
 * Copyright (C) 2024, Bruce MacKinnon KC1FSZ
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
 *
 * NOT FOR COMMERCIAL USE WITHOUT PERMISSION.
 */
#ifndef _CircularQueuePtr_h
#define _CircularQueuePtr_h

#include "Common.h"

namespace kc1fsz {

class CircularQueuePtr {
public:
    
    CircularQueuePtr(uint32_t size) : _size(size) { }

    bool isEmpty() const { return _readPtr == _writePtr; }

    uint32_t getReadPtr() const { return _readPtr; }

    uint32_t getAndIncReadPtr() { 
        uint32_t p = _readPtr;
        uint32_t next = p + 1;
        // Wrap
        if (next == _size) {
            next = 0;
        }
        _readPtr = next;
        _depth--;
        return p;
    }

    uint32_t getWritePtr() const { return _writePtr; }

    uint32_t getAndIncWritePtr() { 
        uint32_t p = _writePtr;
        uint32_t next = p + 1;
        // Wrap
        if (next == _size) {
            next = 0;
        }
        // Check for overflow
        if (next == _writePtr) {
            _overflowCount++;
        }  else {
            _writePtr = next;
            _depth++;
            _maxDepth = std::max(_depth, _maxDepth);
        }
        return p;
    }

    uint32_t getOverflowCount() const { return _overflowCount; }

private:

    const uint32_t _size;
    volatile uint32_t _readPtr = 0;
    volatile uint32_t _writePtr = 0;
    volatile uint32_t _overflowCount = 0;
    volatile uint32_t _depth = 0;
    volatile uint32_t _maxDepth = 0;
};

}

#endif